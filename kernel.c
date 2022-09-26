// ninth/kernel.c
// Copyright (C) 2022 J. M. Spivey

#define PORTABLE 1
#include "ninth.h"

#define push(val) *sp-- = acc; acc = val
#define binary(w) acc = sp[1] w acc; sp++
#define get(ty) acc = * (ty *) acc
#define put(ty) * (ty *) acc = sp[1]; sp += 2; acc = *sp

void trace(int *sp, unsigned *rp, def *w) {
     printf("--");
     for (int *p = (int *) sbase - 1; p >= sp; p--)
          printf(" %d", *p);
     printf(" : [%d] %s\n",
            (unsigned *) &rstack[RSTACK] - rp, def_name(w));
}

void run(int m) {
     int *sp;
     ushort *ip;
     def *w;
     int acc = 0, tmp;

quit:
     sp = sbase;
     rp = rbase;
     ip = (ushort *) defn(m)->d_data;

     while (1) {
          if (sp > (int *) sbase) {
               underflow();
               goto quit;
          }

          w = defn(*ip++);

     reswitch:
          *sp = acc;
          if (tracing) trace(sp, rp, w);

          switch (w->d_action) {
          case A_NOP:
               break;

          case A_QUIT:
               goto quit;

          case A_UNKNOWN:
               *sp-- = acc;
               acc = (int) def_name(w);
               w = defn(UNKNOWN);
               goto reswitch;

          case A_ENTER:
               *--rp = (unsigned) ip;
               ip = (ushort *) w->d_data;
               break;

          case A_EXIT:
               ip = (ushort *) *rp++;
               break;

          case A_EXECUTE:
               w = defn(acc); acc = *++sp; goto reswitch;

          case A_CALL:
               *sp = acc;
               sp = ((int *(*)(int *)) w->d_data)(sp);
               acc = *sp;
               break;

          case A_DONE:
               return;

          case A_ZERO: push(0); break;
          case A_ONE: push(1); break;
          case A_TWO: push(2); break;
          case A_THREE: push(3); break;
          case A_FOUR: push(4); break;
          case A_ADD: binary(+); break;
          case A_SUB: binary(-); break;
          case A_MUL: binary(*); break;
          case A_INC: acc++; break;
          case A_DEC: acc--; break;
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
               tmp = sp[1]; sp[1] = acc; acc = sp[2]; sp[2] = tmp;
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
          case A_VAR:
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
