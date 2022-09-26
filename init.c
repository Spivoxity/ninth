// init.c

#define PORTABLE 1
#define EXTERN
#include "ninth.h"

byte mem[MEMSIZE];
byte *rstack[RSTACK];
char inbuf[INBUF];
char pad[PAD];

int dict, UNKNOWN;
byte dmem[MEMSIZE];
byte *bp;

// underflow -- report stack underflow
void underflow(void) {
     printf("\nStack underflow!\n");
}

// header -- append dictionary entry
static int header(char *name) {
     def *p;

     if (defbase != NULL) {
          printf("Creating %s in mid-stream\n", name);
          exit(1);
     }

     dp = ALIGN(dp, 4);
     p = (def *) dp;
     dp += sizeof(def);

     p->d_next = dict;
     p->d_flags = 0;
     p->d_action = A_UNKNOWN;
     p->d_data = 0;
     strcpy((char *) dp, name);
     dp += strlen(name) + 1;

     bp = ALIGN(bp, 4);
     * (def **) bp = p;
     dict = (bp - mem)/4;
     bp += 4;
     return dict;
}

// find -- find dictionary entry
int find(char *name) {
     for (int d = dict; d >= 0; d = defn(d)->d_next) {
          if (strcmp(name, def_name(defn(d))) == 0)
               return d;
     }

     return -1;
}

// create -- find dictionary entry or create one
int create(char *name) {
     int d = find(name);
     if (d >= 0) return d;
     return header(name);
}

void prim_action(char *name, int action) {
     int d = create(name);
     defn(d)->d_action = action;
}

void _prim_subr(char *name, unsigned subr, char *sym) {
     int d = create(name);
     defn(d)->d_action = A_CALL;
     defn(d)->d_data = subr;
     defsym(defn(d), subr, sym);
}

#define prim_subr(name, subr) _prim_subr(name, (unsigned) subr, #subr)

void _defvar(char *name, unsigned addr, char *sym) {
     int d = create(name);
     defn(d)->d_action = A_VAR;
     defn(d)->d_data = addr;
     defsym(defn(d), addr, sym);
}

#define defvar(name, addr) _defvar(name, (unsigned) &addr, "&" #addr)

void _defconst(char *name, unsigned val, char *sym) {
     int d = create(name);
     defn(d)->d_action = A_CONST;
     defn(d)->d_data = val;
     defsym(defn(d), val, sym);
}

#define defconst(name, val) _defconst(name, val, #val)

ushort W(char *name) {
     // Timing matters here: we will evaluate calls to W() before
     // any surrounding call to assemble(), so we need not worry about
     // interfering with a partial definition
     return create(name);
}

#define L(n) W("(lit)"), n

#define END 0xdeadbeef

void assemble(char *name, ...) {
     int d = create(name);
     byte *base;
     unsigned tok;
     va_list va;

     dp = ALIGN(dp, 4);
     base = dp;

     va_start(va, name);
     while (1) {
          tok = va_arg(va, unsigned);
          if (tok == END) break;
          * (ushort *) dp = tok;
          dp += sizeof(ushort);
     } 
     * (ushort *) dp = W("e_n_d");
     dp += sizeof(ushort);
     va_end(va);

     defn(d)->d_action = A_ENTER;
     defn(d)->d_data = (unsigned) base;
}

void immediate(char *name) {
     int d = create(name);
     defn(d)->d_flags |= IMMED;
}

void init(void) {
#define action(s, p) prim_action(s, p);
#define action0(p)
     ACTIONS(action, action0)

     defvar("state", state);
     defvar("bp", bp);
     defvar("dp", dp);
     defvar("rp", rp);
     defvar("inp", inp);
     defvar("defbase", defbase);
     defvar("tracing", tracing);
     defvar("dict", dict);
     defvar("pad", pad);
     defvar("MEM", mem);
     defvar("args", args);
     defvar("interactive", interactive);

     defconst("ENTER", A_ENTER);
     defconst("CONST", A_CONST);
     defconst("VAR", A_VAR);
     defconst("UNKNOWN", A_UNKNOWN);
     defconst("MEMSIZE", MEMSIZE);

#define __SUBR(name, func)  prim_subr(name, func);
     PRIMS(__SUBR)

     // These are defined as NOPs so they can be redefined in system.nth
     prim_action("?colon", A_NOP);
     prim_action("?comp", A_POP);
     prim_action("init-locals", A_NOP);
     prim_action("pop-locals", A_NOP);
     prim_action("unknown", A_QUIT);
     prim_subr("genword", p_gentok);
     assemble("?tag", W("pop"), W("pop"), END);

     // : : immediate ?colon 1 state ! word create align dp @ defbase !
     //   init-locals ['] : ;
     assemble(":",
              W("?colon"), W("1"), W("state"), W("!"), 
              W("word"), W("create"), W("align"),
              W("dp"), W("@"), W("defbase"), W("!"), W("init-locals"),
              W("(lit)"), W(":"), END);
     immediate(":");

     // : ; immediate ['] : ?tag pop-locals 0 gentok 0 state ! 
     //   ENTER defbase @ defword 0 defbase ! ;
     assemble(";",
              W("(lit)"), W(":"), W("?tag"), W("pop-locals"),
              W("0"), W("gentok"), W("0"), W("state"), W("!"),
              W("ENTER"), W("defbase"), W("@"), W("defword"), 
              W("0"), W("defbase"), W("!"), END);
     immediate(";");

     // : litnum dup dup 16 lsl 16 asr = if
     //    quote (lit) gentok else quote (lit2) dup gentok 16 lsr gentok fi ;
     assemble("litnum", 
              W("dup"), W("dup"), L(16), W("lsl"), L(16), W("asr"),
              W("="), W("(branch0)"), 6,
              W("(lit)"), W("(lit)"), W("gentok"), W("gentok"),
              W("(branch)"), 9, W("(lit)"), W("(lit2)"), W("gentok"),
              W("dup"), W("gentok"),
              L(16), W("lsr"), W("gentok"), END);

     // : immword dup immed? if execute else genword fi ;
     assemble("immword",
              W("dup"), W("immed?"), W("(branch0)"), 3,
              W("execute"), W("(branch)"), 1, W("genword"), END);

     // : compile find if immword else number if litnum
     //    else create gentok fi fi ;
     assemble("compile",
              W("find"), W("(branch0)"), 3, W("immword"), W("(branch)"), 8,
              W("number"), W("(branch0)"), 3, W("litnum"), W("(branch)"), 2,
              W("create"), W("gentok"), END);

     // : interp find if execute else number if else unknown fi fi ;
     assemble("interp",
              W("find"), W("(branch0)"), 3, W("execute"), W("(branch)"), 6,
              W("number"), W("(branch0)"), 2, W("(branch)"), 1,
              W("unknown"), END);

     // : repl do word dup ch@ while 
     //    state @ if compile else interp fi fi od pop
     assemble("repl", 
              W("word"), W("dup"), W("ch@"), W("(branch0)"), 10,
              W("state"), W("@"), W("(branch0)"), 3,
              W("compile"), W("(branch)"), -12, 
              W("interp"), W("(branch)"), -15,
              W("pop"), END);

     // : main 0 state ! do accept inp @ while repl od (done)
     assemble("main",
              W("0"), W("state"), W("!"),
              W("accept"), W("inp"), W("@"),
              W("(branch0)"), 3, W("repl"), W("(branch)"), -8,
              W("(done)"), END);
}

int main(void) {
     dict = -1;
     bp = mem;
     dp = dmem;
     sbase = &mem[MEMSIZE - 4];
     rbase = &rstack[RSTACK];
     init();
     run(find("main"));
     dump();
}
