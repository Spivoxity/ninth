/* Define DUMP to be able to assemble a ROM, or BOOT to boot from
   a ROM image, or neither for an empty interpreter */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

typedef unsigned char uchar;
typedef unsigned short ushort;

#define INBUF 256
char inbuf[INBUF];

// The RAM of the machine is represented by the mem array
// The dictionary must go in mem so that definitions can be accessed
// using two-byte indices; there's no real reason for the other stuff
// to be in there, except to allow the value stack and the dictionary
// to grow towards each other.

#define MEMSIZE 32768

uchar mem[MEMSIZE];

#define ALIGN(p, n) ((uchar *) (((unsigned) (p)+(n)-1) & ~((n)-1)))

// At the bottom of mem is the dictionary area.  There's a chain of def 
// records, each followed the word they it defines, represented as a 
// null-terminated string.  These definitions are interleaved with 
// static data, and each def record has a pointer to its data.

#define dbase 0

typedef struct {
     short d_next;              // Next record in the dictionary
     ushort d_flags;            // Attributes of the word
     unsigned d_execute;        // Action when the word is executed
     uchar *d_data;             // Pointer to the word's body or data
} def;

#define def(t) ((def *) &mem[t])
#define tok(d) ((uchar *) (d) - mem)
#define def_name(d) ((char *) ((d)+1))

#define IMMED 1

// The d_execute field is either one of a fixed list of small integers,
// or a pointer to something more exciting.

enum { 
     P_UNKNOWN, P_ENTER, P_EXIT, P_GET, P_PUT, P_CHGET, P_CHPUT,
     P_TOKGET, P_TOKPUT, P_QUIT,
     P_ADD, P_SUB, P_MUL, P_DIV, P_MOD, P_EQ, P_LESS,
     P_LSL, P_LSR, P_ASR, P_AND, P_OR, P_XOR,
     P_DUP, P_POP, P_SWAP, P_ROT, P_PICK, P_DEPTH,
     P_BRANCH0, P_BRANCH, P_LIT, P_LIT2, P_EXECUTE, P_CALL, P_VAR, P_CONST, 
     P_GENTOK, P_IMMED, P_NOP
};

#define RSTACK 1024

int dict = -1;
unsigned state = 0;
uchar *dp = mem;
uchar *rstack[RSTACK];

#ifdef DUMP
uchar tmem[MEMSIZE];
uchar *tp = tmem;
#else
#define tp dp
#endif


// Primitives

// create -- append dictionary entry
def *create(char *name) {
     def *p;

     dp = ALIGN(dp, 4);
     p = (def *) dp;
     dp += sizeof(def);
     strcpy(dp, name);
     dp += strlen(name)+1;
     dp = ALIGN(dp, 4);

     p->d_next = dict;
     p->d_flags = 0;
     p->d_execute = P_UNKNOWN;
     p->d_data = NULL;
     dict = tok(p);

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
     return create(name);
}


// Threaded interpreter

int *sp;
unsigned *rp;
char *inp;
int trace = 0;

void show_stack(void) {
     int *p;

     for (p = sp; p < (int *) &mem[MEMSIZE]; p++)
          printf(" %d", *p);
}

void run(def *m) {
     ushort *ip;
     def *w;
     int t;
     unsigned x;

quit:
     printf("NINTH\n");
     sp = (int *) &mem[MEMSIZE]; 
     rp = (unsigned *) &rstack[RSTACK];
     ip = (ushort *) m->d_data;
     state = 0;

     while (1) {
          w = (def *) &mem[*ip++];
     reswitch:
          x = w->d_execute;

          if ((uchar *) sp > &mem[MEMSIZE]) {
               printf("\nStack underflow!\n");
               goto quit;
          }

          if (trace) {
               printf("--");
               show_stack();
               printf(" : %s\n", def_name(w));
          }

          switch (x) {
          case P_NOP:
               break;

          case P_QUIT:
               goto quit;
               break;

          case P_UNKNOWN:
               printf("%s is unknown\n", def_name(w));
               goto quit;
               break;

          case P_ENTER:
               *--rp = (unsigned) ip;
               ip = (ushort *) w->d_data;
               break;

          case P_EXIT:
               if (rp >= (unsigned *) &rstack[RSTACK]) return;
               ip = (ushort *) *rp++;
               break;

#define binary(w) sp[1] = sp[1] w sp[0]; sp++; break;

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

          case P_AND:
               binary(&); break;

          case P_LSL:
               binary(<<); break;

          case P_LSR:
               sp[1] = (unsigned) sp[1] >> sp[0]; sp++; break;

          case P_ASR:
               sp[1] = (int) sp[1] >> sp[0]; sp++; break;

          case P_OR:
               binary(|); break;

          case P_XOR:
               binary(^); break;

               // GET and PUT are sensitive to evaluation order when
               // the address accessed is an alias of sp.

          case P_GET:
               sp[0] = * (int *) sp[0]; break;

          case P_PUT:
               * (int *) sp[0] = sp[1]; sp += 2; break;

          case P_CHGET:
               sp[0] = * (char *) sp[0]; break;

          case P_CHPUT:
               * (char *) sp[0] = sp[1]; sp += 2; break;

          case P_TOKGET:
               sp[0] = * (short *) sp[0]; break;

          case P_TOKPUT:
               * (short *) sp[0] = sp[1]; sp += 2; break;

          case P_DUP:
               sp--; sp[0] = sp[1]; break;

          case P_PICK:
               sp[0] = sp[sp[0]+1]; break;

          case P_DEPTH:
               t = (int *) &mem[MEMSIZE] - sp; *--sp = t;
               break;

          case P_POP:
               sp++; break;

          case P_SWAP:
               t = sp[0]; sp[0] = sp[1]; sp[1] = t;
               break;

          case P_ROT:
               t = sp[2]; sp[2] = sp[1]; sp[1] = sp[0]; sp[0] = t;
               break;

          case P_BRANCH0:
               t = (signed short) *ip++;
               if (*sp++ == 0) ip += t;
               break;

          case P_BRANCH:
               t = (signed short) *ip++; ip += t; break;

          case P_LIT:
               *--sp = (signed short) *ip++; break;

          case P_LIT2:
               *--sp = (ip[1] << 16) + ip[0]; ip += 2; break;

          case P_EXECUTE:
               w = (def *) &mem[*sp++];
               goto reswitch;

          case P_CALL:
               t = (int) w->d_data;
               (* (void (*)(void)) t)();
               break;

          case P_VAR:
               *--sp = (int) w->d_data;
               break;

          case P_CONST:
               *--sp = * (int *) w->d_data;
               break;

          case P_GENTOK:
               * (short *) tp = *sp++;
               tp += sizeof(short);
               break;

          case P_IMMED:
               sp[0] = ((((def *) &mem[sp[0]])->d_flags & IMMED) != 0);
               break;

          default:
               printf("Unknown action\n");
               goto quit;
          }
     }
}


// Primitives as subroutines

uchar *defbase = NULL;

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
          *sp = (unsigned) tok(d);
          *--sp = 1;
     }
}

void p_bl(void) {
     while (*inp != '\0' && isspace(*inp)) inp++;
}     

void p_scan(void) {
     char *base = (char *) sp[0];
     char delim = sp[1];
     char *p = base;

     while (*inp != '\0' && *inp != delim)
          *p++ = *inp++;
     if (*inp != '\0') inp++;
     *p++ = '\0';
     
     sp[1] = (unsigned) base; sp++;
}     

void p_word(void) {
     char *base = (char *) dp;
     char *p = base;

     while (*inp != '\0' && isspace(*inp)) inp++;
     while (*inp != '\0' && !isspace(*inp))
          *p++ = *inp++;
     if (*inp != '\0') inp++;
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

void p_putc(void) {
     putchar(*sp++);
}

void p_stack(void) {
     int *p;

     for (p = (int *) sp; p < (int *) &mem[MEMSIZE]; p++) {
          printf(" %d", *p);
     }

     printf("\n");
}

void p_create(void) {
     char name[32];
     strcpy(name, (char *) sp[0]); // Save from scratch area

#ifdef DUMP
     def *d = find_create(name);
#else
     def *d = find(name);

     if (d == NULL) {
          if (defbase == NULL) 
               d = create(name);
          else {
               printf("The Earth moved for me\n");

               // Calculate required space
               unsigned a = 
                    ALIGN(defbase+sizeof(def)+strlen(name)+1, 4) - defbase;
               unsigned b = dp - defbase;

               printf("defbase = %u, a = %u, b = %u\n",
                      (unsigned) defbase, a, b);

               // Move partial code out of the way
               memmove(defbase+a, defbase, b);

               // Reset dp and create the definition
               dp = defbase;
               d = create(name);
               printf("d = %u\n", (unsigned) d);
               assert((uchar *) d == defbase);
               assert(dp == defbase+a);

               // Set dp after the moved code
               defbase = defbase+a;
               dp = defbase+b;
          }
     }
#endif
     sp[0] = tok(d);
}

void p_defword(void) {
     def *d = (def *) &mem[sp[2]];
     unsigned action = sp[1];
     uchar *data = (uchar *) sp[0];
     d->d_execute = action;
     d->d_data = data;
     sp += 3;

     printf("%s defined\n", def_name(d));
}

void p_immediate(void) {
     def *d = (def *) &mem[sp[0]];
     d->d_flags |= IMMED;
}

#include <setjmp.h>

jmp_buf finish;

void p_accept(void) {
     printf("> "); fflush(stdout);
     if (fgets(inbuf, INBUF, stdin) == NULL)
          longjmp(finish, 1);
#ifdef DUMP
     printf("%s", inbuf);
#endif
     inp = inbuf;
}


// Bootstrapping

#ifdef BOOT

#define sym(x) (int) x
#include "boot.c"

#else

#define MAXSYM 100
uchar *addrs[MAXSYM];
char *syms[MAXSYM];
int nsyms = 0;

void defsym(uchar *addr, char *sym) {
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
     defsym((uchar *) subr, sym);
}

#define prim_subr(name, subr) _prim_subr(name, subr, #subr)

void _defvar(char *name, uchar *addr, char *sym) {
     def *d = find_create(name);
     d->d_execute = P_VAR;
     d->d_data = addr;
     defsym(addr, sym);
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
     primitive("and", P_AND);
     primitive("or", P_OR);
     primitive("xor", P_XOR);
     primitive("dup", P_DUP);
     primitive("pick", P_PICK);
     primitive("depth", P_DEPTH);
     primitive("pop", P_POP);
     primitive("swap", P_SWAP);
     primitive("rot", P_ROT);
     primitive("branch0", P_BRANCH0);
     primitive("branch", P_BRANCH);
     primitive("lit", P_LIT);
     primitive("lsl", P_LSL);
     primitive("lsr", P_LSR);
     primitive("asr", P_ASR);
     primitive("lit2", P_LIT2);
     primitive("exit", P_EXIT);
     prim_subr(".", p_dot);
     prim_subr("nl", p_nl);
     prim_subr("word", p_word);
     prim_subr("bl", p_bl);
     prim_subr("scan", p_scan);
     prim_subr("find", p_find);
     primitive("execute", P_EXECUTE);
     prim_subr("number", p_number);
     prim_subr("putc", p_putc);
     prim_subr("stack", p_stack);
     defvar("state", &state);
     defvar("dp", &dp);
     defvar("tp", &tp);
     defvar("rp", &rp);
     defvar("base", &defbase);
     defvar("trace", &trace);
     defvar("dict", &dict);
     primitive("gentok", P_GENTOK);
     defconst("ENTER", P_ENTER);
     defconst("EXIT", P_EXIT);
     defconst("VAR", P_VAR);
     defconst("CONST", P_CONST);
     defvar("MEM", mem);
     prim_subr("[create]", p_create);
     primitive("immed?", P_IMMED);
     prim_subr("defword", p_defword);
     prim_subr("immediate", p_immediate);
     immediate("immediate");
     primitive("?comp", P_NOP);
     primitive("?interp", P_NOP);
     prim_subr("accept", p_accept);
     primitive("quit", P_QUIT);

     assemble("not", L(0), W("="), EXIT);

     // : : immediate ?interp 1 state ! word [create] tp @ base ! ;
     assemble(":",
              W("?interp"), L(1), W("state"), W("!"), 
              W("word"), W("[create]"), 
              W("tp"), W("@"), W("base"), W("!"), EXIT);
     immediate(":");

     // : ; immediate ?comp quote exit 0 state ! ENTER base @ defword 
     //    0 base !;
     assemble(";",
              W("?comp"), W("lit"), W("exit"), W("gentok"), 
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
              W("accept"), W("repl"), W("branch"), -4, EXIT);
}
#endif


// DUMP

#ifdef DUMP
#define MAXDEFS 200

def *defs[MAXDEFS];
int ndefs = 0;

void map_defs(void) {
     int d = dict;

     while (d >= 0) {
          defs[ndefs++] = def(d);
          d = def(d)->d_next;
     }
}

void dump_mem(void) {
     uchar *p = mem;
     int n = ndefs-1;
     int k = 0;

     printf("#define BOOTMEM %d\n\n", (dp - mem)/4);
     printf("unsigned boot[BOOTMEM] = {\n");

     while (p < dp) {
          printf("/* %4d */ ", p - mem);

          if (n >= 0 && p == (uchar *) defs[n]) {
               def *d = (def *) p;
               printf("%d, %d, ", 
                      (d->d_next & 0xffff) + (d->d_flags << 16), 
                      d->d_execute);

               if (k < nsyms && d->d_data == addrs[k])
                    printf("sym(%s),", syms[k++]);
               else if (d->d_data >= mem && d->d_data < dp)
                    printf("sym(&mem[%d]),", d->d_data - mem);
               else if (d->d_data >= tmem && d->d_data < tp)
                    printf("sym(&rom[%d]),", (d->d_data - tmem)/2);
               else
                    printf("%d,", (int) d->d_data);

               printf(" /* %s */\n", def_name(d));
               p += sizeof(def); n--;
          }
          else {
               printf("%u,\n", * (unsigned *) p);
               p += sizeof(unsigned);
          }
     }

     printf("};\n\n");
}

void dump_rom(void) {
     short *p = (short *) tmem;

     printf("#define ROM %d\n\n", (tp - tmem)/2);
     printf("short rom[ROM] = {\n");

     while (p < (short *) tp) {
          int n = 0;
          printf("/* %4d */", p - (short *) tmem); 
          while (n++ < 8 && p < (short *) tp)
               printf(" %d,", *p++);
          printf("\n");
     }

     printf("};\n\n");
}

void dump(void) {
     int i;

     printf("// --boot\n");
     map_defs();
     dump_rom();
     dump_mem();
     printf("#define DICT %d\n", dict);
}
#endif

int main(void) {
#ifdef BOOT
     dp = &mem[BOOTMEM * sizeof(unsigned)];
     memcpy(mem, boot, dp - mem);
     dict = DICT;
#else
     init_dict();
#endif

     if (! setjmp(finish))
          run(find("main"));

     printf("\nBye\n");

#ifdef DUMP
     dump();
#endif

     return 0;
}
