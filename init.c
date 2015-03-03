/* init.c */

#include "ninth.h"

// Dictionary

// header -- append dictionary entry
def *header(char *name) {
     def *p;

     if (defbase != NULL) {
          printf("Attempting to create %s in mid-stream\n", name);
          exit(1);
     }

     dp = ALIGN(dp, 4);
     p = (def *) dp;
     dp += sizeof(def);

     p->d_next = dict;
     p->d_flags = 0;
     p->d_action = P_UNKNOWN;
     p->d_data = NULL;
     dict = tok(p);

     strcpy((char *) dp, name);
     dp = ALIGN(dp+strlen(name)+1, 4);

     return p;
}

// find -- find dictionary entry
def *find(char *name) {
     int d = dict;

     while (d >= 0) {
          if (strcmp(name, def_name(def(d))) == 0)
               return def(d);
          d = def(d)->d_next;
     }

     return NULL;
}

// create -- find dictionary entry or create if needed
def *create(char *name) {
     def *d = find(name);
     if (d != NULL) return d;
     return header(name);
}


// Bootstrap

#define MAXSYM 100
def *sdefs[MAXSYM];
uchar *addrs[MAXSYM];
char *syms[MAXSYM];
int nsyms = 0;

void defsym(def *d, uchar *addr, char *sym) {
     sdefs[nsyms] = d;
     addrs[nsyms] = addr;
     syms[nsyms] = sym;
     nsyms++;
}

void primitive(char *name, int token) {
     def *d = create(name);
     d->d_action = token;
}

void _prim_subr(char *name, void (*subr)(void), char *sym) {
     def *d = create(name);
     d->d_action = P_CALL;
     d->d_data = (uchar *) subr;
     defsym(d, (uchar *) subr, sym);
}

#define prim_subr(name, subr) _prim_subr(name, subr, #subr)

void _defvar(char *name, uchar *addr, char *sym) {
     def *d = create(name);
     d->d_action = P_VAR;
     d->d_data = addr;
     defsym(d, addr, sym);
}

#define defvar(name, addr) _defvar(name, (uchar *) addr, #addr)

void defconst(char *name, unsigned val) {
     def *d = create(name);
     d->d_action = P_CONST;
     d->d_data = dp;
     * (unsigned *) dp = val;
     dp += sizeof(unsigned);
}

ushort W(char *name) {
     // Timing matters here: we will evaluate calls to W() before
     // any surrounding call to assemble(), so we need not worry about
     // interfering with a partial definition
     
     def *d = create(name);
     return (uchar *) d - mem;
}

#define L(n) W("lit"), n

#define END 0xdeadbeef

void assemble(char *name, ...) {
     def *d = create(name);
     uchar *base = tp;
     unsigned tok;
     va_list va;

     va_start(va, name);
     while (1) {
          tok = va_arg(va, unsigned);
          if (tok == END) break;
          * (ushort *) tp = tok;
          tp += sizeof(ushort);
     } 
     * (ushort *) tp = W("e_n_d");
     tp += sizeof(ushort);
     va_end(va);

     d->d_action = P_ENTER;
     d->d_data = base;
}

void immediate(char *name) {
     def *d = create(name);
     d->d_flags |= IMMED;
}

void init_dict(void) {
#undef prim
#undef prim0
#define prim(s, p) primitive(s, p);
#define prim0(p)
PRIMS

     primitive("exit", P_EXIT);

     prim_subr(".", p_dot);
     prim_subr("word", p_word);
     prim_subr("scan", p_scan);
     prim_subr("find", p_find);
     prim_subr("number", p_number);
     prim_subr("putc", p_putc);
     prim_subr("create", p_create);
     prim_subr("defword", p_defword);
     prim_subr("accept", p_accept);
     prim_subr("immed?", p_immed);
     prim_subr("gentok", p_gentok);
     prim_subr("strcmp", p_strcmp);

     defvar("state", &state);
     defvar("dp", &dp);
     defvar("tp", &tp);
     defvar("rp", &rp);
     defvar("inp", &inp);
     defvar("base", &defbase);
     defvar("trace", &trace);
     defvar("dict", &dict);
     defvar("pad", &pad);
     defvar("MEM", mem);
     defvar("MEMEND", &mem[MEMSIZE]);

     defconst("DEFTAG", 16180);
     defconst("ENTER", P_ENTER);
     defconst("VAR", P_VAR);
     defconst("CONST", P_CONST);

     // These are defined as NOP so they can be redefined in system.nth
     primitive("?colon", P_NOP);
     primitive("?comp", P_POP);
     primitive("banner", P_NOP);

     // assemble("not", L(0), W("="), END);
     assemble("?tag", W("pop"), W("pop"), END);

     // : : immediate interp 1 state ! word create tp @ base ! DEFTAG ;
     assemble(":",
              W("?colon"), L(1), W("state"), W("!"), 
              W("word"), W("create"),
              W("tp"), W("@"), W("base"), W("!"), W("DEFTAG"), END);
     immediate(":");

     // : ; immediate DEFTAG ?tag quote exit 0 state ! 
     //    ENTER base @ defword 0 base ! ;
     assemble(";",
              W("DEFTAG"), W("?tag"), W("lit"), W("e_n_d"), W("gentok"), 
              L(0), W("state"), W("!"),
              W("ENTER"), W("base"), W("@"), W("defword"), 
              L(0), W("base"), W("!"), END);
     immediate(";");

     // : litnum dup dup 16 lsl 16 asr = if
     //    quote lit gentok else quote lit2 dup gentok 16 lsr gentok fi ;
     assemble("litnum", 
              W("dup"), W("dup"), L(16), W("lsl"), L(16), W("asr"),
              W("="), W("branch0"), 5,
              W("lit"), W("lit"), W("gentok"), W("gentok"), W("exit"),
              W("lit"), W("lit2"), W("gentok"), W("dup"), W("gentok"),
              L(16), W("lsr"), W("gentok"), END);

     // : immword dup immed? if execute else gentok fi ;
     assemble("immword",
              W("dup"), W("immed?"), W("branch0"), 2,
              W("execute"), W("exit"), W("gentok"), END);

     // : compile find if immword else number if litnum
     //    else create gentok fi fi ;
     assemble("compile",
              W("find"), W("branch0"), 2, W("immword"), W("exit"),
              W("number"), W("branch0"), 2, W("litnum"), W("exit"),
              W("create"), W("gentok"), END);

     // : interp find if execute else number not if unknown fi fi ;
     assemble("interp",
              W("find"), W("branch0"), 2, W("execute"), W("exit"),
              W("number"), W("not"), W("branch0"), 1,
              W("unknown"), END);

     // : repl do word dup ch@ while 
     //    state @ if compile else interp fi fi od pop
     assemble("repl", 
              W("word"), W("dup"), W("ch@"), W("branch0"), 10,
              W("state"), W("@"), W("branch0"), 3,
              W("compile"), W("branch"), -12, 
              W("interp"), W("branch"), -15,
              W("pop"), END);

     // : main do accept repl od
     assemble("main",
              W("banner"), 
              W("accept"), W("repl"), W("branch"), -4, END);
}
