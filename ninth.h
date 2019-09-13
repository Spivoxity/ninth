// ninth.h

/* Parameters */

#define INBUF 256
#define PAD 32
#define MEMSIZE 32768
#define RSTACK 1024

#define SBASE (MEMSIZE-4)

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <setjmp.h>
#include <ctype.h>

typedef unsigned char byte;
typedef unsigned short ushort;

/* Dictionary entry.  The spelling of the name follows. */
typedef struct {
     short d_next;              // Token for next definition, or -1
     ushort d_flags;            // Attributes of the word
     unsigned d_action;         // Action when executed
     void *d_data;              // Pointer to the word's body or data
} def;

/* In the portable version of Ninth, the d_action field selects a case
   in the interpreter run(kernel.c), but it could become an index into
   a despatch table of assembly-language routines, or the address of
   such a routine. */

/* A 'token' is a 16-bit index into the 'mem' array, leading to
   pointer to the definition of a word; the definition itself can live
   outside the 'mem' array, e.g. in ROM.  Because pointers must be
   4-bit aligned, we can scale the index appropriately. */
#define defn(t)  ((def **) mem)[t]

/* The name of a word, given its definition */
#define def_name(d)  ((char *) ((d)+1))

/* Flag bits */
#define IMMED 1

/* List of actions: those listed with action are entered in the
   initial dictionary, and those listed with action0 are not.  UNKNOWN
   must come first, so it is at index 0, and E_N_D must come second,
   so that it corresponds to token 0. */
#define ACTIONS(action, action0) \
     action0(A_UNKNOWN) action("e_n_d", A_EXIT) action0(A_NOP) \
     action("quit", A_QUIT) action0(A_ENTER) action("+", A_ADD) \
     action("-", A_SUB) action("*", A_MUL) action("/", A_DIV) \
     action("mod", A_MOD) action("=", A_EQ) action("<", A_LESS) \
     action("u<", A_ULESS) action("and", A_AND) action("lsl", A_LSL) \
     action("lsr", A_LSR) action("asr", A_ASR) action("or", A_OR) \
     action("xor", A_XOR) action("@", A_GET) action("!", A_PUT) \
     action("ch@", A_CHGET) action("ch!", A_CHPUT) \
     action("tok@", A_TOKGET) action("tok!", A_TOKPUT) \
     action("dup", A_DUP) action("?dup", A_QDUP) action("pick", A_PICK) \
     action("roll", A_ROLL) action("pop", A_POP) \
     action("swap", A_SWAP) action("r>", A_RPOP) action(">r", A_RPUSH) \
     action("r@", A_RAT) action("rot", A_ROT) action("branch0", A_BRANCH0) \
     action("branch", A_BRANCH) action("lit", A_LIT) action("lit2", A_LIT2) \
     action("execute", A_EXECUTE) action0(A_CALL) action0(A_VAR) \
     action0(A_CONST)action("over", A_OVER) action("tuck", A_TUCK) \
     action("nip", A_NIP) action("not", A_NOT)

/* The ACTIONS macro is used several times, and here is the first: an
   enumerated type. */
enum {
#define __ENUM(s, p) p,
#define __ENUM0(p) p,
     ACTIONS(__ENUM, __ENUM0)
     N_ACTIONS
};

/* Primitives implemented as subroutines */
#define PRIMS(prim) \
     prim(".", p_dot) \
     prim("word", p_word) \
     prim("scan", p_scan) \
     prim("find", p_find) \
     prim("number", p_number) \
     prim("putc", p_putc) \
     prim("create", p_create) \
     prim("gentok", p_gentok) \
     prim("defword", p_defword) \
     prim("redirect", p_redirect) \
     prim("accept", p_accept) \
     prim("immed?", p_immed) \
     prim("align", p_align) \
     prim("strcmp", p_strcmp) \
     prim("depth", p_depth) \
     prim("MEMSIZE", p_memsize) \
     prim("f+", p_fadd) \
     prim("f-", p_fsub) \
     prim("f*", p_fmul) \
     prim("f/", p_fdiv) \
     prim("f<", p_fless) \
     prim("sqrt", p_sqrt) \
     prim("sin", p_sin) \
     prim("cos", p_cos) \
     prim("tan", p_tan) \
     prim("exp", p_exp) \
     prim("log", p_log) \
     prim("f.", p_fdot) \
     prim("float", p_float) \
     prim("entier", p_entier) \
     prim("atan2", p_atan2)

#define __DECL(name, func) \
     int *func(int *);
PRIMS(__DECL)


/* Global variables */
#ifndef EXTERN
#define EXTERN extern
#endif

extern byte mem[];
extern char inbuf[];
extern char pad[];

EXTERN byte *defbase;
EXTERN byte *dp;
EXTERN int dict;
EXTERN int trace;
EXTERN char *inp;
EXTERN int state;
EXTERN unsigned *rp;
EXTERN char **args;
EXTERN int phase;
EXTERN int nlocals;
EXTERN int locbase;
extern short locvar[];

#ifdef INIT
extern byte dmem[];
EXTERN byte *bp;
#else
#define bp dp
#endif

#define ALIGN(p, n) ((byte *) (((unsigned) (p)+(n)-1) & ~((n)-1)))


/* kernel.c */
void run(int m);

/* init.c */
int create(char *name);
int find(char *name);
void init(void);

/* dump.c */
void defsym(def *d, byte *addr, char *sym);
void dump(void);

/* boot.c */
#define sym(x) ((int) x)
#define heading(next, flags, action) \
     (next & 0xffff) | (flags << 16), action

extern const unsigned boot[];
extern const unsigned BOOTSIZE;
extern const int MAIN;
