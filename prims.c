#include "ninth.h"

void p_putc(void) {
     putchar(*sp++);
}

void p_accept(void) {
#ifndef DUMP
     printf("> "); 
#endif
     fflush(stdout);
     if (fgets(inbuf, INBUF, stdin) == NULL)
          longjmp(finish, 1);
     inp = inbuf;
}

void p_dot(void) {
     printf(" %d", (int) *sp++);
}

void p_number(void) {
     // ( string -- number 1 ) or ( string -- string 0 )

     char *end;
     int n = strtol((char *) *sp, &end, 0);
     if (*end != '\0')
          *--sp = 0;
     else {
          *sp = n;
          *--sp = 1;
     }
}

#ifndef BOOT
void p_word(void) {
     char *p = pad;

     while (*inp != '\0' && isspace(*inp)) inp++;
     while (*inp != '\0' && !isspace(*inp))
          *p++ = *inp++;
     if (*inp != '\0') inp++;
     *p = '\0';
     
     *--sp = (int) pad;
}

void p_defword(void) {
     def *d = (def *) &mem[sp[2]];
     unsigned action = sp[1];
     uchar *data = (uchar *) sp[0];
     d->d_execute = action;
     d->d_data = data;
     sp += 3;

     printf("%s defined\n", def_name(d));
}

void p_create(void) {
     char *name = (char *) sp[0];
     def *d = find_create(name);
     sp[0] = tok(d);
}

void p_find(void) {
     // ( string -- def 1 ) or ( string -- string 0 )
     def *d = find((char *) *sp);
     if (d == NULL)
          *--sp = 0;
     else {
          *sp = (unsigned) tok(d);
          *--sp = 1;
     }
}

void p_scan(void) {
     char *base = (char *) sp[0];
     char delim = sp[1];
     char *p = base;

     while (*inp != '\0' && *inp != delim)
          *p++ = *inp++;
     if (*inp != '\0') inp++;
     *p++ = '\0';
     
     sp[1] = (unsigned) base; sp++;
}     
#endif
