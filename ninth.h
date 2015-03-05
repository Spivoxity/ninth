/* ninth.h */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <setjmp.h>
#include <ctype.h>

#define INBUF 256
#define PAD 32
#define MEMSIZE 32768
#define RSTACK 1024

typedef unsigned char uchar;
typedef unsigned short ushort;

#define ALIGN(p, n) ((uchar *) (((unsigned) (p)+(n)-1) & ~((n)-1)))

extern uchar mem[];
extern uchar *rstack[];
extern char inbuf[];
extern char pad[];
extern int dict;
extern unsigned state;
extern uchar *defbase;
extern uchar *dp;
extern int *sp;
extern unsigned *rp;
extern int trace;
extern int memsize;
#ifdef DUMP
extern uchar tmem[];
extern uchar *tp;
#else
#define tp dp
#endif
extern char *inp;

// At the bottom of mem is the dictionary area.  There's a chain of def 
// records, each followed the word they it defines, represented as a 
// null-terminated string.  These definitions are interleaved with 
// static data, and each def record has a pointer to its data.

typedef struct {
     short d_next;              // Next record in the dictionary
     uchar d_flags;             // Attributes of the word
     uchar d_action;           // Action when the word is executed
     uchar *d_data;             // Pointer to the word's body or data
} def;

#define def(t) ((def *) &mem[t])
#define tok(d) ((uchar *) (d) - mem)
#define def_name(d) ((char *) ((d)+1))

#define IMMED 1

// UNKNOWN must come first, so it has d_action = 0
// END must come second, so it has address 0
#define PRIMS \
     prim0(P_UNKNOWN) prim("e_n_d", P_EXIT) prim0(P_NOP) \
     prim("quit", P_QUIT) prim0(P_ENTER) prim("+", P_ADD) \
     prim("-", P_SUB) prim("*", P_MUL) prim("/", P_DIV) prim("mod", P_MOD) \
     prim("=", P_EQ) prim("<", P_LESS) prim("u<", P_ULESS) \
     prim("and", P_AND) prim("lsl", P_LSL) prim("lsr", P_LSR) \
     prim("asr", P_ASR) prim("or", P_OR) prim("xor", P_XOR) \
     prim("@", P_GET) prim("!", P_PUT) prim("ch@", P_CHGET) \
     prim("ch!", P_CHPUT) prim("tok@", P_TOKGET) prim("tok!", P_TOKPUT) \
     prim("dup", P_DUP) prim("pick", P_PICK) prim("roll", P_ROLL) \
     prim("depth", P_DEPTH) prim("pop", P_POP) prim("swap", P_SWAP) \
     prim("r>", P_RPOP) prim(">r", P_RPUSH) prim("r@", P_RAT) \
     prim("rot", P_ROT) prim("branch0", P_BRANCH0) prim("branch", P_BRANCH) \
     prim("lit", P_LIT) prim("lit2", P_LIT2) prim("execute", P_EXECUTE) \
     prim0(P_CALL) prim0(P_VAR) prim0(P_CONST) \
     prim("over", P_OVER) prim("tuck", P_TUCK) prim("nip", P_NIP) \
     prim("not", P_NOT) prim("MEM+", P_MEMPLUS)

enum { 
#define prim(s, p) p,
#define prim0(p) p,
PRIMS
};

// prims.c
void p_find(void);
void p_dot(void);
void p_scan(void);
void p_word(void);
void p_number(void);
void p_putc(void);
void p_memmove(void);
void p_accept(void);
void p_create(void);
void p_defword(void);
void p_immed(void);
void p_gentok(void);
void p_strcmp(void);

// init.c
extern def *sdefs[];
extern uchar *addrs[];
extern char *syms[];
extern int nsyms;

def *create(char *name);
def *find(char *name);
void init_dict(void);

// dump.c
void dump(void);

// kernel.c
extern jmp_buf finish;

// boot.c
extern unsigned boot[];
extern unsigned BOOTSIZE;
extern ushort DICT, MAIN;

#define sym(x) (int) x
#define heading(next, flags, action) \
     (next & 0xffff) | (flags << 16) | (action << 24)
