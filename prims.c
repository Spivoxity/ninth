#include "ninth.h"
#include "math.h"

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

#define float_fun(name, fun) \
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

int *p_accept(int *sp) {
#ifndef INIT
     printf("> "); 
#endif
     fflush(stdout);
     if (fgets(inbuf, INBUF, stdin) == NULL) {
          printf("\nBye\n");
          exit(0);
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

int *p_memsize(int *sp) {
     *--sp = MEMSIZE;
     return sp;
}

#ifdef INIT
int *p_immed(int *sp) {
     sp[0] = ((defn(sp[0])->d_flags & IMMED) != 0);
     return sp;
}

int *p_word(int *sp) {
     char *p = pad;

     while (*inp != '\0' && isspace(*inp)) inp++;
     while (*inp != '\0' && !isspace(*inp))
          *p++ = *inp++;
     if (*inp != '\0') inp++;
     *p = '\0';
     
     *--sp = (int) pad;
     return sp;
}

int *p_gentok(int *sp) {
     * (short *) dp = *sp++; dp += sizeof(short);
     return sp;
}

int *p_align(int *sp) {
     dp = ALIGN(dp, 4);
     return sp;
}

int *p_defword(int *sp) {
     def *d = defn(sp[2]);
     int action = sp[1];
     byte *data = (byte *) sp[0];
     d->d_action = action;
     d->d_data = data;
     sp += 3;

     printf("%s defined\n", def_name(d));
     return sp;
}

int *p_create(int *sp) {
     char *name = (char *) sp[0];
     sp[0] = create(name);
     return sp;
}

int *p_find(int *sp) {
     // ( string -- def 1 ) or ( string -- string 0 )
     int d = find((char *) *sp);
     if (d < 0)
          *--sp = 0;
     else {
          *sp = d;
          *--sp = 1;
     }
     return sp;
}

int *p_scan(int *sp) {
     char *base = (char *) sp[0];
     char delim = sp[1];
     char *p = base;

     while (*inp != '\0' && *inp != delim)
          *p++ = *inp++;
     if (*inp != '\0') inp++;
     *p++ = '\0';
     
     sp[1] = (int) base; sp++;
     return sp;
}     
#endif
