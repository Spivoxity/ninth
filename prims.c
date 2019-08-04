#include "ninth.h"

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
     int n = strtol((char *) *sp, &end, 0);
     if (*end != '\0')
          *--sp = 0;
     else {
          *sp = n;
          *--sp = 1;
     }
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
