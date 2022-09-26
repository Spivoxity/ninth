#include "ninth.h"

int *p_div(int *sp) {
     sp[1] = sp[1] / sp[0];
     sp++; return sp;
}

int *p_mod(int *sp) {
     sp[1] = sp[1] % sp[0];
     sp++; return sp;
}

int *p_roll(int *sp) {
     int n = *sp++, tmp;
     if (n >= 0) {
          tmp = sp[n];
          while (n > 0) { sp[n] = sp[n-1]; n--; }
          sp[0] = tmp;
     } else {
          int *ep = sp - n;
          tmp = ep[n];
          while (n < 0) { ep[n] = ep[n+1]; n++; }
          ep[0] = tmp;
     }
     return sp;
}

#define FLO(a) (* (float *) (a))

#define float_op(name, op) \
     int *name(int *sp) { \
          FLO(&sp[1]) = FLO(&sp[1]) op FLO(&sp[0]); \
          sp++; return sp; \
     }

float_op(p_fadd, +)
float_op(p_fsub, -)
float_op(p_fmul, *)
float_op(p_fdiv, /)

int *p_fless(int *sp) {
     sp[1] = (FLO(&sp[1]) < FLO(&sp[0]));
     sp++; return sp;
}

#include "math.h"

#define float_fun(name, fun)					\
     int *name(int *sp) { FLO(sp) = fun(FLO(sp)); return sp; }

float_fun(p_sqrt, sqrtf)
float_fun(p_sin, sinf)
float_fun(p_cos, cosf)
float_fun(p_tan, tanf)
float_fun(p_exp, expf)
float_fun(p_log, logf)

int *p_fdot(int *sp) {
     printf("%f ", FLO(sp++));
     return sp;
}

int *p_float(int *sp) { FLO(sp) = (float) sp[0]; return sp; }
int *p_entier(int *sp) { sp[0] = (int) FLO(sp); return sp; }

int *p_atan2(int *sp) { 
     FLO(&sp[1]) = atan2f(FLO(&sp[1]), FLO(&sp[0])); 
     sp++;
     return sp;
}

int *p_putc(int *sp) {
     putchar(*sp++);
     return sp;
}

FILE *infp = NULL;

int *p_redirect(int *sp) {
     char *fname = (char *) *sp++;

     infp = fopen(fname, "r");
     if (infp == NULL) {
          fprintf(stderr, "%s: cannot open\n", fname);
          exit(2);
     }

     return sp;
}

int *p_accept(int *sp) {
     FILE *fp = infp;

     if (fp == NULL) {
          if (interactive) {
               printf("> "); 
               fflush(stdout);
          }
          fp = stdin;
     }

     if (fgets(inbuf, INBUF, fp) == NULL) {
          if (infp != NULL) {
               fclose(infp);
               infp = NULL;
          }
          
          inp = NULL;
          return sp;
     }

     inp = inbuf;
     return sp;
}

int *p_dot(int *sp) {
     printf("%d ", *sp++);
     return sp;
}

int *p_number(int *sp) {
     // ( string -- number 1 ) or ( string -- string 0 )
     char *end;
     int n;
     float f;

     n = strtol((char *) *sp, &end, 0);
     if (*end == '\0') {
          *sp = n;
          *--sp = 1;
          return sp;
     }

     f = strtof((char *) *sp, &end);
     if (*end == '\0') {
          FLO(sp) = f;
          *--sp = 1;
          return sp;
     }
     
     *--sp = 0;
     return sp;
}

int *p_strcmp(int *sp) {
     sp[1] = strcmp((char *) sp[1], (char *) sp[0]);
     sp++;
     return sp;
}

int *p_depth(int *sp) {
     int r = (int *) sbase - sp;
     *--sp = r;
     return sp;
}
