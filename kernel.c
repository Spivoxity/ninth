// kernel.c: primitive actions

#define PORTABLE 1
#include "ninth.h"

void show_stack(int *sp) {
     for (int *p = sp; p < (int *) &mem[SBASE]; p++)
          printf(" %d", *p);
}

#define push(val) *sp-- = acc; acc = val
#define binary(w) acc = sp[1] w acc; sp++
#define get(ty) acc = * (ty *) acc
#define put(ty) * (ty *) acc = sp[1]; sp += 2; acc = *sp

void run(int m) {
     int *sp;
     ushort *ip;
     def *w;
     int acc = 0, tmp;

quit:
     sp = (int *) &mem[SBASE];
     rp = (unsigned *) &rstack[RSTACK];
     ip = (ushort *) defn(m)->d_data;

     while (1) {
          if ((byte *) sp > &mem[SBASE]) {
               printf("\nStack underflow!\n");
               goto quit;
          }

          w = defn(*ip++);

     reswitch:
          if (trace && w->d_action != A_EXIT) {
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

          case A_ZERO: push(0); break;
          case A_ONE: push(1); break;
          case A_TWO: push(2); break;
          case A_THREE: push(3); break;
          case A_FOUR: push(4); break;
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
               if (acc >= 0) {
                    sp++;
                    tmp = sp[acc];
                    while (acc > 0) { sp[acc] = sp[acc-1]; acc--; }
                    acc = tmp;
               } else {
                    acc = -acc; tmp = 0;
                    while (tmp <= acc) { sp[tmp] = sp[tmp+1]; tmp++; }
                    sp[acc+1] = sp[0];
                    acc = *++sp;
               }
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
               push(*rp++);
               break;

          case A_RPUSH:
               *--rp = acc; acc = *++sp;
               break;

          case A_RAT:
               push(*rp);
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
               push((signed short) *ip++);
               break;

          case A_LIT2:
               push((ip[1] << 16) + ip[0]); ip += 2;
               break;

          case A_CONST:
               push(w->d_data);
               break;

          case A_LOCALS:
               /* The stack contains (x1 x2 ... xn) with n at ip:
                  transfer (x1 x2 ... xn) to Rstack in reverse order. */
               tmp = *ip++;
               while (tmp > 0) {
                    *--rp = acc;
                    acc = *++sp;
                    tmp--;
               }
               break;
        
          case A_GETLOC:
               push(rp[*ip++]);
               break;

          case A_SETLOC:
               rp[*ip++] = acc;
               acc = *++sp;
               break;

          case A_POPLOCS:
               rp += *ip++;
               break;

          default:
               printf("Unknown action %d for %s\n",
                      w->d_action, def_name(w));
               goto quit;
          }
     }
}
