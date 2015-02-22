#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

typedef unsigned char uchar;
typedef unsigned short ushort;

// The RAM of the machine is represented by the mem array
// The dictionary must go in mem so that definitions can be accessed
// using two-byte indices; there's no real reason for the other stuff
// to be in there, except to allow the value stack and the dictionary
// to grow towards each other.  Beware, however that there must not be
// a definition at index 0; we could change that assumption.

#define MEMSIZE 32768

uchar mem[MEMSIZE];

#define ALIGN(p, n) ((uchar *) (((unsigned) (p)+(n)-1) & ~(n-1)))

// At the bottom of mem is a buffer for the current input string
char *inbuf = (char *) &mem[0];
#define INBUF 256

// Next comes the dictionary area.  There's a chain of def records,
// each followed the word they define, represented as a null-terminated 
// string.  These definitions are interleaved with static data, and each
// def record has a pointer to its data.

#define dbase INBUF

typedef struct {
     ushort d_next;             // Next record in the dictionary
     ushort d_flags;            // Attributes of the word
     unsigned d_execute;        // Action when the word is executed
     uchar *d_data;             // Pointer to the word's body or data
} def;

#define IMMED 1

#define def_name(d) ((char *) ((d)+1))

// The d_execute field is either one of a fixed list of small integers,
// or a pointer to something more exciting.

enum { 
     P_UNKNOWN, P_ENTER, P_EXIT, P_GET, P_PUT, P_CHGET, P_CHPUT,
     P_TOKGET, P_TOKPUT,
     P_ADD, P_SUB, P_MUL, P_DIV, P_MOD, P_EQ, P_LESS, P_DUP, P_POP, 
     P_SWAP, P_ROT,
     P_BRANCH0, P_BRANCH, P_LIT, P_EXECUTE, P_CALL, P_VAR, P_CONST, 
     P_GENTOK, P_IMMED
};

def *dict = NULL;
uchar *dp = &mem[dbase];

// At the top end of memory is the return stack, and below it, growing
// towards the dictionary, is the eval stack.  The return stack has
// four-byte entries.

#define RSTACK 256

#define rbase MEMSIZE
#define sbase (MEMSIZE - RSTACK * sizeof(unsigned))


// Primitives

// create -- append dictionary entry
def *create(char *name) {
     short a = ALIGN(dp, 4) - mem;
     def *p = (def *) &mem[a];
     
     dp = &mem[a + sizeof(def)];
     strcpy(dp, name);
     dp += strlen(name)+1;
     dp = ALIGN(dp, 2);

     p->d_next = (dict != NULL ? (uchar *) dict - mem : 0);
     p->d_flags = 0;
     p->d_execute = P_UNKNOWN;
     p->d_data = NULL;

     dict = (def *) &mem[a];
     return p;
}

#define def_next(d) (d->d_next != 0 ? (def *) &mem[d->d_next] : NULL)

// find -- find dictionary entry
def *find(char *name) {
     def *d = dict;

     while (d != NULL) {
          if (strcmp(name, def_name(d)) == 0)
               return d;
          d = def_next(d);
     }

     return NULL;
}

// find_create -- find dictionary entry or create if needed
def *find_create(char *name) {
     def *d = find(name);
     if (d != NULL) return d;
     return create(name);
}

// names -- print list of names 
void names() {
     def *d = dict;

     while (d != NULL) {
          printf(" %s", def_name(d));
          d = def_next(d);
     }

     printf("\n");
}


// Threaded interpreter

unsigned *sp;
unsigned *rp;
char *inp;
int trace = 0;

void init(void) {
     sp = (unsigned *) &mem[sbase]; 
     rp = (unsigned *) &mem[rbase];
}

void dump(void) {
     int *p;

     for (p = (int *) sp; p < (int *) &mem[sbase]; p++)
          printf(" %d", *p);
}

void run(def *m) {
     ushort *ip;
     def *w;
     unsigned t, x;

     ip = (ushort *) m->d_data;

     while (1) {
          w = (def *) &mem[*ip++];
     reswitch:
          x = w->d_execute;

          if (trace) {
               printf("--");
               dump();
               printf(" : %s\n", def_name(w));
          }

          switch (x) {
          case P_UNKNOWN:
               printf("%s is unknown\n", def_name(w));
               exit(1);
               break;

          case P_ENTER:
               *--rp = (unsigned) ip;
               ip = (ushort *) w->d_data;
               break;

          case P_EXIT:
               if (rp >= (unsigned *) &mem[rbase]) return;
               ip = (ushort *) *rp++;
               break;

#define binary(w) sp[1] = (unsigned) ((int) sp[1] w (int) sp[0]); sp++; break;

          case P_ADD:
               binary(+); break;

          case P_SUB:
               binary(-); break;
               
          case P_MUL:
               binary(*); break;

          case P_DIV:
               binary(/); break;

          case P_MOD:
               binary(%); break;

          case P_EQ:
               binary(==); break;

          case P_LESS:
               binary(<); break;

          case P_GET:
               sp[0] = * (unsigned *) sp[0]; break;

          case P_PUT:
               * (unsigned *) sp[0] = sp[1]; sp += 2; break;

          case P_CHGET:
               sp[0] = * (char *) sp[0]; break;

          case P_CHPUT:
               * (char *) sp[0] = sp[1]; sp += 2; break;

          case P_TOKGET:
               sp[0] = (int) * (signed short *) sp[0]; break;

          case P_TOKPUT:
               * (short *) sp[0] = sp[1]; sp += 2; break;

          case P_DUP:
               sp--; sp[0] = sp[1]; break;

          case P_POP:
               sp++; break;

          case P_SWAP:
               t = sp[0]; sp[0] = sp[1]; sp[1] = t;
               break;

          case P_ROT:
               t = sp[2]; sp[2] = sp[1]; sp[1] = sp[0]; sp[0] = t;
               break;

          case P_BRANCH0:
               t = (int) (signed short) *ip++;
               if (*sp++ == 0) ip += t;
               break;

          case P_BRANCH:
               t = (int) (signed short) *ip++; ip += t; break;

          case P_LIT:
               *--sp = (unsigned) (int) (signed short) *ip++; break;

          case P_EXECUTE:
               w = (def *) *sp++;
               goto reswitch;

          case P_CALL:
               t = (unsigned) w->d_data;
               (* (void (*)(void)) t)();
               break;

          case P_VAR:
               *--sp = (unsigned) w->d_data;
               break;

          case P_CONST:
               *--sp = * (unsigned *) w->d_data;
               break;

          case P_GENTOK:
               * (short *) dp = *sp++;
               dp += sizeof(short);
               break;

          case P_IMMED:
               sp[0] = ((((def *) sp[0])->d_flags & IMMED) != 0);
               break;

          default:
               printf("Unknown action\n");
               exit(1);
          }
     }
}


// Primitives as subroutines

void p_dot(void) {
     printf(" %d", (int) *sp++);
}

void p_nl(void) {
     printf("\n");
}

void p_find(void) {
     // ( string -- def 1 ) or ( string -- string 0 )
     def *d = find((char *) *sp);
     if (d == NULL)
          *--sp = 0;
     else {
          *sp = (unsigned) d; 
          *--sp = 1;
     }
}

void p_bl(void) {
     while (*inp != '\0' && isspace(*inp)) inp++;
}     

void p_scan(void) {
     char delim = sp[0];
     char *base = (char *) dp;
     char *p = base;

     while (*inp != '\0' && *inp != delim)
          *p++ = *inp++;
     *p++ = '\0';
     
     sp[0] = (unsigned) base;
}     

void p_word(void) {
     char *base = (char *) dp;
     char *p = base;

     while (*inp != '\0' && isspace(*inp)) inp++;
     while (*inp != '\0' && !isspace(*inp))
          *p++ = *inp++;
     *p++ = '\0';
     
     *--sp = (unsigned) base;
}

void p_number(void) {
     // ( string -- number 1 ) or ( string -- string 0 )

     char *end;
     int n = strtol((char *) *sp, &end, 0);
     if (*end != '\0')
          *--sp = 0;
     else {
          *sp = (unsigned) n;
          *--sp = 1;
     }
}

void p_puts(void) {
     printf("%s\n", (char *) *sp++);
}

void p_stack(void) {
     int *p;

     for (p = (int *) sp; p < (int *) &mem[sbase]; p++) {
          printf(" %d", *p);
     }

     printf("\n");
}

void p_create(void) {
     // [create] should preserve a partial definition is one is present.

     // ( string -- newdef )
     char name[32];

     strcpy(name, (char *) sp[0]); // Save from scratch area
     sp[0] = (unsigned) find_create(name);
}

void p_defword(void) {
     def *d = (def *) sp[2];
     unsigned action = sp[1];
     uchar *data = (uchar *) sp[0];
     d->d_execute = action;
     d->d_data = data;
     sp += 3;
}

void p_immediate(void) {
     def *d = (def *) sp[0];
     d->d_flags |= IMMED;
}


// Bootstrapping

unsigned state = 0;
uchar *base = NULL;

void primitive(char *name, int token) {
     def *d = find_create(name);
     d->d_execute = token;
}

void prim_subr(char *name, void (*subr)(void)) {
     def *d = find_create(name);
     d->d_execute = P_CALL;
     d->d_data = (uchar *) subr;
}

void defvar(char *name, uchar *addr) {
     def *d = find_create(name);
     d->d_execute = P_VAR;
     d->d_data = addr;
}

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
     uchar *base = dp;
     unsigned tok;
     va_list va;

     va_start(va, name);
     while (1) {
          tok = va_arg(va, unsigned);
          if (tok == EXIT) break;
          * (ushort *) dp = tok;
          dp += sizeof(ushort);
     } 
     * (ushort *) dp = W("exit");
     dp += sizeof(ushort);
     va_end(va);

     d->d_execute = P_ENTER;
     d->d_data = base;
}

void immediate(char *name) {
     def *d = find_create(name);
     d->d_flags |= IMMED;
}

void bootstrap(void) {
     primitive("@", P_GET);
     primitive("!", P_PUT);
     primitive("ch@", P_CHGET);
     primitive("ch!", P_CHPUT);
     primitive("tok@", P_TOKGET);
     primitive("tok!", P_TOKPUT);
     primitive("+", P_ADD);
     primitive("-", P_SUB);
     primitive("*", P_MUL);
     primitive("/", P_DIV);
     primitive("mod", P_MOD);
     primitive("=", P_EQ);
     primitive("<", P_LESS);
     primitive("dup", P_DUP);
     primitive("pop", P_POP);
     primitive("swap", P_SWAP);
     primitive("rot", P_ROT);
     primitive("branch0", P_BRANCH0);
     primitive("branch", P_BRANCH);
     primitive("lit", P_LIT);
     primitive("exit", P_EXIT);
     prim_subr(".", p_dot);
     prim_subr("nl", p_nl);
     prim_subr("word", p_word);
     prim_subr("bl", p_bl);
     prim_subr("scan", p_scan);
     prim_subr("find", p_find);
     primitive("execute", P_EXECUTE);
     prim_subr("number", p_number);
     prim_subr("puts", p_puts);
     prim_subr("stack", p_stack);
     defvar("state", (uchar *) &state);
     defvar("dp", (uchar *) &dp);
     defvar("rp", (uchar *) &rp);
     defvar("base", (uchar *) &base);
     defvar("trace", (uchar *) &trace);
     primitive("gentok", P_GENTOK);
     defconst("ENTER", P_ENTER);
     defconst("MEM", (unsigned) mem);
     prim_subr("[create]", p_create);
     primitive("immed?", P_IMMED);
     prim_subr("defword", p_defword);
     defvar("inp", (uchar *) inp);
     prim_subr("immediate", p_immediate);
     immediate("immediate");

     assemble("1-", L(1), W("-"), EXIT);
     assemble("not", L(0), W("="), EXIT);

     assemble("fac", 
              W("dup"), W("branch0"), 6,
              W("dup"), W("1-"), W("fac"), W("*"), W("branch"), 3,
              W("pop"), L(1),
              EXIT);

     // : genword MEM - gentok ;
     assemble("genword", W("MEM"), W("-"), W("gentok"), EXIT);

     // : : 1 state ! word [create] dp @ base ! ;
     assemble(":",
              L(1), W("state"), W("!"), W("word"), W("[create]"),
              W("dp"), W("@"), W("base"), W("!"), EXIT);

     // : ; immediate ['] exit gentok 0 state ! ENTER base @ defword 
     //    0 base !;
     assemble(";",
              L(W("exit")), W("gentok"), L(0), W("state"), W("!"),
              W("ENTER"), W("base"), W("@"), W("defword"),
              L(0), W("base"), W("!"), EXIT);
     immediate(";");

     // litnum should use lit2 when needed
     // : litnum ['] lit genword gentok ;
     assemble("litnum", 
              L(W("lit")), W("gentok"), W("gentok"), EXIT);

     // : immword dup immed? if execute else genword fi ;
     assemble("immword",
              W("dup"), W("immed?"), W("branch0"), 3,
              W("execute"), W("branch"), 1,
              W("genword"), EXIT);

     // : compile find if immword else number if litnum
     //    else [create] genword fi fi ;
     assemble("compile",
              W("find"), W("branch0"), 3,
              W("immword"), W("branch"), 8,
              W("number"), W("branch0"), 3,
              W("litnum"), W("branch"), 2,
              /* W("[create]"), W("genword"), */
              W("abort"), W("abort"),
              EXIT);

     // : interp find if execute else number not if quit fi fi ;
     assemble("interp",
              W("find"), W("branch0"), 3,
              W("execute"), W("branch"), 5,
              W("number"), W("not"), W("branch0"), 1,
              W("quit"), EXIT);

     // : main do word dup ch@ while 
     //    state @ if compile else interp fi fi od pop
     assemble("main", 
              W("word"), W("dup"), W("ch@"), W("branch0"), 10,
              W("state"), W("@"), W("branch0"), 3,
              W("compile"), W("branch"), -12, 
              W("interp"), W("branch"), -15,
              W("pop"), EXIT);
     
     // : ['] immediate ['] lit gentok word [create] genword ;
     assemble("[']", 
              L(W("lit")), W("gentok"), 
              W("word"), W("[create]"), W("genword"), EXIT);
     immediate("[']");
}

int main(void) {
     // trace = 1;
     bootstrap();
     init();
     names();
     while (1) {
          inp = (char *) mem;
          if (fgets(inp, INBUF, stdin) == NULL) break;
          fputs(inp, stdout);
          run(find("main"));
     }
     return 0;
}
