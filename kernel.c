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

#define SBASE (MEMSIZE-4)       // One word safety margin for acc

#ifdef PROF
unsigned ticks = 0;
unsigned prof[MEMSIZE/4];
#endif

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

     for (p = sp; p < (int *) &mem[SBASE]; p++)
          printf(" %d", *p);
}

#define binary(w) acc = sp[1] w acc; sp++
#define get(t) acc = * (t *) acc
#define put(t) * (t *) acc = sp[1]; sp += 2; acc = *sp

void run(def *m) {
     ushort *ip;
     def *w;
     int acc = 0, t;

quit:
     sp = (int *) &mem[SBASE]; 
     rp = (unsigned *) &rstack[RSTACK];
     ip = (ushort *) m->d_data;
     state = 0;
     defbase = NULL;

     while (1) {
          if ((uchar *) sp > &mem[SBASE]) {
               printf("\nStack underflow!\n");
               goto quit;
          }

          w = (def *) &mem[*ip++];

     reswitch:

#ifdef PROF
          ticks++;
          prof[(int *) w - (int *) mem]++;
#endif

          if (trace) {
               *sp = acc;
               printf("--");
               show_stack();
               printf(" : %s\n", def_name(w));
          }

          switch (w->d_execute) {
          case P_NOP:
               break;

          case P_QUIT:
               goto quit;

          case P_UNKNOWN:
               printf("%s is unknown\n", def_name(w)); 
               goto quit;

          case P_ENTER:
               *--rp = (unsigned) ip;
               ip = (ushort *) w->d_data;
               break;

          case P_EXIT:
          case P_END:
               if (rp >= (unsigned *) &rstack[RSTACK]) return;
               ip = (ushort *) *rp++;
               break;

          case P_ADD: binary(+); break;
          case P_SUB: binary(-); break;
          case P_MUL: binary(*); break;
          case P_DIV: binary(/); break;
          case P_MOD: binary(%); break;
          case P_EQ: binary(==); break;
          case P_LESS: binary(<); break;
          case P_AND: binary(&); break;
          case P_LSL: binary(<<); break;
          case P_ASR: binary(>>); break;
          case P_OR: binary(|); break;
          case P_XOR: binary(^); break;

          case P_ULESS:
               acc = (unsigned) sp[1] < (unsigned) acc; sp++;
               break;

          case P_LSR:
               acc = (unsigned) sp[1] >> acc; sp++;
               break;

          case P_GET: get(int); break;
          case P_PUT: put(int); break;
          case P_CHGET: get(char); break;
          case P_CHPUT: put(char); break;
          case P_TOKGET: get(short); break;
          case P_TOKPUT: put(short); break;

          case P_DUP:
               sp--; sp[1] = acc;
               break;

          case P_OVER:
               *sp-- = acc; acc = sp[2];
               break;

          case P_PICK:
               acc = sp[acc+1];
               break;

          case P_ROT:
               t = sp[2]; sp[2] = sp[1]; sp[1] = acc; acc = t;
               break;

          case P_ROLL:
               sp++;
               if (acc <= 0) { acc = *sp; break; }
               t = sp[acc];
               while (acc > 0) { sp[acc] = sp[acc-1]; acc--; }
               acc = t;
               break;

          case P_DEPTH:
               *sp = acc; acc = (int *) &mem[SBASE] - sp; --sp;
               break;

          case P_POP:
               acc = *++sp;
               break;

          case P_SWAP:
               t = acc; acc = sp[1]; sp[1] = t;
               break;

          case P_TUCK:
               sp--; sp[1] = sp[2]; sp[2] = acc;
               break;

          case P_NIP:
               sp++;
               break;

          case P_RPOP:
               *sp = acc; acc = *rp++; sp--;
               break;

          case P_RPUSH:
               *--rp = acc; acc = *++sp;
               break;

          case P_RAT:
               *sp-- = acc; acc = *rp;
               break;

          case P_BRANCH0:
               t = (short) *ip++; 
               if (acc == 0) ip += t;
               acc = *++sp;
               break;

          case P_BRANCH:
               t = (short) *ip++; ip += t;
               break;

          case P_LIT:
               *sp-- = acc; acc = (signed short) *ip++;
               break;

          case P_LIT2:
               *sp-- = acc; acc = (ip[1] << 16) + ip[0]; ip += 2;
               break;

          case P_EXECUTE:
               w = (def *) &mem[acc]; acc = *++sp; goto reswitch;

          case P_CALL:
               *sp = acc;
               (* (void (*)(void)) w->d_data)();
               acc = *sp;
               break;

          case P_VAR:
               *sp-- = acc; acc = (int) w->d_data;
               break;

          case P_CONST:
               *sp-- = acc; acc = * (int *) w->d_data;
               break;

          case P_NOT:
               acc = ! acc;
               break;

          case P_MEMPLUS:
               acc = (int) &mem[acc];
               break;

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

#ifdef PROF
void prof_dump(void) {
     int i;

     for (i = 0; i < MEMSIZE/4; i++) {
          if (prof[i] > 0) {
               def *d = (def *) &mem[4*i];
               printf("%u %s\n", prof[i], def_name(d));
          }
     }
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
          run(MAIN);

#ifdef PROF
     printf("\nBye (%u ticks)\n", ticks);
     prof_dump();
#else
     printf("\nBye\n");
#endif

#ifdef DUMP
     dump();
#endif

     return 0;
}
