// kernel.c: primitive actions

#define EXTERN
#include "ninth.h"

byte mem[MEMSIZE];
byte *rstack[RSTACK];
char inbuf[INBUF];
char pad[PAD];

#ifdef INIT
byte dmem[MEMSIZE];
#endif

#define SBASE (MEMSIZE-4)

void show_stack(int *sp) {
     for (int *p = sp; p < (int *) &mem[SBASE]; p++)
          printf(" %d", *p);
}

#define binary(w) acc = sp[1] w acc; sp++
#define get(ty) acc = * (ty *) acc
#define put(ty) * (ty *) acc = sp[1]; sp += 2; acc = *sp

static void run(def *m) {
     int *sp;
     ushort *ip;
     def *w;
     int acc = 0, tmp;

quit:
     sp = (int *) &mem[SBASE];
     rp = (unsigned *) &rstack[RSTACK];
     ip = (ushort *) m->d_data;

     while (1) {
          if ((byte *) sp > &mem[SBASE]) {
               printf("\nStack underflow!\n");
               goto quit;
          }

          w = defn(*ip++);

     reswitch:
          if (trace) {
               *sp = acc;
               printf("--");
               show_stack(sp);
               printf(" : [%d] %s\n",
                      (unsigned *) &rstack[RSTACK] - rp, def_name(w));
          }

          switch (w->d_action) {
          case A_NOP:
               break;

          case A_QUIT:
               goto quit;

          case A_UNKNOWN:
               printf("%s is unknown\n", def_name(w));
               goto quit;

          case A_ENTER:
               *--rp = (unsigned) ip;
               ip = (ushort *) w->d_data;
               break;

          case A_EXIT:
               if (rp >= (unsigned *) &rstack[RSTACK]) return;
               ip = (ushort *) *rp++;
               break;

          case A_EXECUTE:
               w = defn(acc); acc = *++sp; goto reswitch;

          case A_CALL:
               *sp = acc;
               sp = ((int *(*)(int *)) w->d_data)(sp);
               acc = *sp;
               break;

          case A_ADD: binary(+); break;
          case A_SUB: binary(-); break;
          case A_MUL: binary(*); break;
          case A_DIV: binary(/); break;
          case A_MOD: binary(%); break;
          case A_EQ: binary(==); break;
          case A_LESS: binary(<); break;
          case A_AND: binary(&); break;
          case A_LSL: binary(<<); break;
          case A_ASR: binary(>>); break;
          case A_OR: binary(|); break;
          case A_XOR: binary(^); break;

          case A_ULESS:
               acc = (unsigned) sp[1] < (unsigned) acc; sp++;
               break;

          case A_LSR:
               acc = (unsigned) sp[1] >> acc; sp++;
               break;

          case A_GET: get(int); break;
          case A_PUT: put(int); break;
          case A_CHGET: get(char); break;
          case A_CHPUT: put(char); break;
          case A_TOKGET: get(short); break;
          case A_TOKPUT: put(short); break;

          case A_DUP:
               sp--; sp[1] = acc;
               break;

          case A_QDUP:
               if (acc) { sp--; sp[1] = acc; }
               break;
               
          case A_OVER:
               *sp-- = acc; acc = sp[2];
               break;

          case A_PICK:
               acc = sp[acc+1];
               break;

          case A_ROT:
               tmp = sp[2]; sp[2] = sp[1]; sp[1] = acc; acc = tmp;
               break;

          case A_ROLL:
               sp++;
               if (acc <= 0) { acc = *sp; break; }
               tmp = sp[acc];
               while (acc > 0) { sp[acc] = sp[acc-1]; acc--; }
               acc = tmp;
               break;

          case A_DEPTH:
               *sp = acc; acc = (int *) &mem[SBASE] - sp; --sp;
               break;

          case A_POP:
               acc = *++sp;
               break;

          case A_SWAP:
               tmp = acc; acc = sp[1]; sp[1] = tmp;
               break;

          case A_TUCK:
               sp--; sp[1] = sp[2]; sp[2] = acc;
               break;

          case A_NIP:
               sp++;
               break;

          case A_RPOP:
               *sp = acc; acc = *rp++; sp--;
               break;

          case A_RPUSH:
               *--rp = acc; acc = *++sp;
               break;

          case A_RAT:
               *sp-- = acc; acc = *rp;
               break;

          case A_BRANCH0:
               tmp = (short) *ip++; 
               if (acc == 0) ip += tmp;
               acc = *++sp;
               break;

          case A_BRANCH:
               tmp = (short) *ip++; ip += tmp;
               break;

          case A_LIT:
               *sp-- = acc; acc = (signed short) *ip++;
               break;

          case A_LIT2:
               *sp-- = acc; acc = (ip[1] << 16) + ip[0]; ip += 2;
               break;

          case A_VAR:
               *sp-- = acc; acc = (int) w->d_data;
               break;

          case A_CONST:
               *sp-- = acc; acc = * (int *) w->d_data;
               break;

          case A_NOT:
               acc = ! acc;
               break;

          case A_MEMPLUS:
               acc = (int) &mem[acc];
               break;

          default:
               printf("Unknown action\n");
               goto quit;
          }
     }
}

int main(void) {
#ifdef INIT
     atexit(dump);
     dict = -1;
     bp = mem;
     dp = dmem;
     init();
     run(defn(find("main")));
#else
     memcpy(mem, boot, BOOTSIZE);
     dp = &mem[BOOTSIZE];
     run(defn(MAIN));
#endif

     return 0;
}
