/* init.c */

#include "ninth.h"

// Dictionary

// header -- append dictionary entry
def *header(char *name) {
     def *p;

     if (defbase != NULL) {
          printf("Attempting to define %s in mid-stream\n", name);
          exit(1);
     }

     dp = ALIGN(dp, 4);
     p = (def *) dp;
     dp += sizeof(def);

     p->d_next = dict;
     p->d_flags = 0;
     p->d_execute = P_UNKNOWN;
     p->d_data = NULL;
     dict = tok(p);

     strcpy((char *) dp, name);
     dp += strlen(name)+1;

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

// find_create -- find dictionary entry or create if needed
def *find_create(char *name) {
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
     def *d = find_create(name);
     d->d_execute = token;
}

void _prim_subr(char *name, void (*subr)(void), char *sym) {
     def *d = find_create(name);
     d->d_execute = P_CALL;
     d->d_data = (uchar *) subr;
     defsym(d, (uchar *) subr, sym);
}

#define prim_subr(name, subr) _prim_subr(name, subr, #subr)

void _defvar(char *name, uchar *addr, char *sym) {
     def *d = find_create(name);
     d->d_execute = P_VAR;
     d->d_data = addr;
     defsym(d, addr, sym);
}

#define defvar(name, addr) _defvar(name, (uchar *) addr, #addr)

void defconst(char *name, unsigned val) {
     def *d = find_create(name);
     d->d_execute = P_CONST;
     d->d_data = dp;
     * (unsigned *) dp = val;
     dp += sizeof(unsigned);
}

ushort W(char *name) {
     // Timing matters here: we will evaluate calls to W() before
     // any surrounding call to assemble(), so we need not worry about
     // interfering with a partial definition
     
     def *d = find_create(name);
     return (uchar *) d - mem;
}

#define L(n) W("lit"), n

#define EXIT 0xdeadbeef

void assemble(char *name, ...) {
     def *d = find_create(name);
     uchar *base = tp;
     unsigned tok;
     va_list va;

     va_start(va, name);
     while (1) {
          tok = va_arg(va, unsigned);
          if (tok == EXIT) break;
          * (ushort *) tp = tok;
          tp += sizeof(ushort);
     } 
     * (ushort *) tp = W("exit");
     tp += sizeof(ushort);
     va_end(va);

     d->d_execute = P_ENTER;
     d->d_data = base;
}

void immediate(char *name) {
     def *d = find_create(name);
     d->d_flags |= IMMED;
}

void init_dict(void) {
#undef prim
#define prim(s, p, a) primitive(s, p);
#undef prim0
#define prim0(p, a)
PRIMS

     prim_subr(".", p_dot);
     prim_subr("word", p_word);
     prim_subr("scan", p_scan);
     prim_subr("find", p_find);
     prim_subr("number", p_number);
     prim_subr("putc", p_putc);
     prim_subr("[create]", p_create);
     prim_subr("defword", p_defword);
     prim_subr("accept", p_accept);

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

     defconst("ENTER", P_ENTER);
     defconst("EXIT", P_EXIT);
     defconst("VAR", P_VAR);
     defconst("CONST", P_CONST);
     defconst("UNKNOWN", P_UNKNOWN);

     // These are defined as NOP so they can be redefined in system.nth
     primitive("?colon", P_NOP);
     primitive("?comp", P_POP);
     primitive("banner", P_NOP);

     assemble("not", L(0), W("="), EXIT);
     assemble("?tag", W("pop"), W("pop"), EXIT);

     // : : immediate interp 1 state ! word [create] tp @ base ! ['] : ;
     assemble(":",
              W("?colon"), L(1), W("state"), W("!"), 
              W("word"), W("[create]"), W("align"),
              W("tp"), W("@"), W("base"), W("!"), L(16180), EXIT);
     immediate(":");

     // : ; immediate 16180 ?tag quote exit 0 state ! 
     //    ENTER base @ defword 0 base ! ;
     assemble(";",
              L(16180), W("?tag"), W("lit"), W("exit"), W("gentok"), 
              L(0), W("state"), W("!"),
              W("ENTER"), W("base"), W("@"), W("defword"), 
              L(0), W("base"), W("!"), EXIT);
     immediate(";");

     // litnum should use lit2 when needed
     // : litnum dup dup 16 lsl 16 asr = if
     //    quote lit gentok else quote lit2 dup gentok 16 lsr gentok fi ;
     assemble("litnum", 
              W("dup"), W("dup"), L(16), W("lsl"), L(16), W("asr"),
              W("="), W("branch0"), 6,
              W("lit"), W("lit"), W("gentok"), W("gentok"), W("branch"), 9,
              W("lit"), W("lit2"), W("gentok"), W("dup"), W("gentok"),
              L(16), W("lsr"), W("gentok"), EXIT);

     // : immword dup immed? if execute else gentok fi ;
     assemble("immword",
              W("dup"), W("immed?"), W("branch0"), 3,
              W("execute"), W("branch"), 1,
              W("gentok"), EXIT);

     // : compile find if immword else number if litnum
     //    else [create] gentok fi fi ;
     assemble("compile",
              W("find"), W("branch0"), 3,
              W("immword"), W("branch"), 8,
              W("number"), W("branch0"), 3,
              W("litnum"), W("branch"), 2,
              W("[create]"), W("gentok"),
              EXIT);

     // : interp find if execute else number not if quit fi fi ;
     assemble("interp",
              W("find"), W("branch0"), 3,
              W("execute"), W("branch"), 5,
              W("number"), W("not"), W("branch0"), 1,
              W("unknown"), EXIT);

     // : repl do word dup ch@ while 
     //    state @ if compile else interp fi fi od pop
     assemble("repl", 
              W("word"), W("dup"), W("ch@"), W("branch0"), 10,
              W("state"), W("@"), W("branch0"), 3,
              W("compile"), W("branch"), -12, 
              W("interp"), W("branch"), -15,
              W("pop"), EXIT);

     // : main do accept repl od
     assemble("main",
              W("banner"), 
              W("accept"), W("repl"), W("branch"), -4, EXIT);
}
