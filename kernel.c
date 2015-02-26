/* Define DUMP to be able to assemble a ROM, or BOOT to boot from
   a ROM image, or neither for an empty interpreter */

#include "ninth.h"

// The RAM of the machine is represented by the mem array
// The dictionary must go in mem so that definitions can be accessed
// using two-byte indices; there's no real reason for the other stuff
// to be in there, except to allow the value stack and the dictionary
// to grow towards each other.

uchar mem[MEMSIZE];
uchar *rstack[RSTACK];
char inbuf[INBUF];
char pad[PAD];
int dict = -1;
unsigned state = 0;
uchar *defbase = NULL;

// Dp is not kept aligned, and must be aligned explicitly before each 
// definition that needs it.  It is 4-byte aligned before each token
// string to allow for the earth to move. Tp (if it exists) must be 
// kept 2-byte aligned at all times.

uchar *dp = mem;

#ifdef DUMP
uchar tmem[MEMSIZE];            // Prototype ROM
uchar *tp = tmem;
#endif

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
     sp = (int *) &mem[MEMSIZE]; 
     rp = (unsigned *) &rstack[RSTACK];
     ip = (ushort *) m->d_data;
     state = 0;
     defbase = NULL;

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
#undef prim0
#define prim0(p, a) case p: a; break;
PRIMS

          default:
               printf("Unknown action\n");
               goto quit;
          }
     }
}


// Main program

#ifdef BOOT
#define sym(x) (int) x
#include "boot.c"
#else
#define MAIN find("main")
#endif

jmp_buf finish;

int main(void) {
#ifdef BOOT
     dp = &mem[BOOTMEM * sizeof(unsigned)];
     memcpy(mem, boot, dp - mem);
     dict = DICT;
#else
     init_dict();
#endif

     if (! setjmp(finish))
          run(MAIN);

     printf("\nBye\n");

#ifdef DUMP
     dump();
#endif

     return 0;
}
