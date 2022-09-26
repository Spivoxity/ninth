// dump2.c

#define PORTABLE 1
#include "ninth.h"

#define MAXDEFS 500
#define MAXSYM 200

def *defs[MAXDEFS];
def *sdefs[MAXSYM];
unsigned addrs[MAXSYM];
char *syms[MAXSYM];
int nsyms = 0, ndefs;

void defsym(def *d, unsigned addr, char *sym) {
     sdefs[nsyms] = d;
     addrs[nsyms] = addr;
     syms[nsyms] = sym;
     nsyms++;
}

void map_defs(void) {
     int d, n;

     ndefs = 0;
     for (d = dict; d >= 0; d = defn(d)->d_next)
          ndefs++;

     n = ndefs;
     for (d = dict; d >= 0; d = defn(d)->d_next)
          defs[--n] = defn(d);

     assert(n == 0);
}

static const char *act_name[] = {
#define __NAME(name, val) #val,
#define __NAME0(val) #val,
     ACTIONS(__NAME, __NAME0)
};

void dump(void) {
     int n, k;
     byte *p;

     printf("# boot.s\n\n");

     printf("\t.ifdef PORTABLE\n");
     for (int i = 0; i < N_ACTIONS; i++)
          printf("\t.equ %s, %d\n", act_name[i], i);
     printf("\t.endif\n\n");

     printf("\t.equ MEMSIZE, %d\n\n", MEMSIZE);

     printf("\t.equ bp, dp\n\n");

     printf("\t.section .rodata\n\n");

     bp = ALIGN(bp, 4);
     dp = ALIGN(dp, 4);
     map_defs();

     printf("\t.p2align 2\n");
     printf("rom:");
     n = k = 0; p = dmem;
     while (p < dp) {
          if (n < ndefs && p == (byte *) defs[n]) {
               def *d = (def *) p;
               printf("\n\t# %s\n", def_name(d));
               printf("\t.short %d\n", d->d_next);
               printf("\t.short %d\n", d->d_flags);
               printf("\t.long %s\n", act_name[d->d_action]);

               if (k < nsyms && d->d_data == addrs[k]) {
		    char *sym = syms[k];
		    if (*sym == '&') sym++;
                    printf("\t.long %s\n", sym);
	       } else if (d->d_action == A_ENTER) {
                    assert((unsigned) d->d_data % 4 == 0);
                    printf("\t.long rom+%d\n", (byte *) d->d_data - dmem);
               } else if (d->d_action == A_VAR) {
                    printf("\t.long mem+%d\n", (byte *) d->d_data - mem);
	       } else {
                    printf("\t.long %#x\n", (int) d->d_data);
	       }

               p += sizeof(def);

               printf("\t.asciz \"");
               while (*p != '\0') {
                    if (*p == '"')
                         printf("\\\"");
                    else
                         printf("%c", *p);

                    p++;
               }
               printf("\"\n");
               printf("\t.p2align 2\n");
               p = ALIGN(p+1, 4);
               
               n++;
               if (d == sdefs[k]) k++;
          } else {
               printf("\t.long %#x\n", * (unsigned *) p);
               p += sizeof(unsigned);
          }
     }
     printf("\n");

     n = 0;
     printf("\t.global boot\n");
     printf("\t.p2align 2\n");
     printf("boot:\n");
     for (unsigned *p = (unsigned *) mem; p < (unsigned *) bp; p++) {
          unsigned v = *p;

          if (n < ndefs && v == (unsigned) defs[n])
               printf("\t.long rom+%d\n", (byte *) v - dmem), n++;
          else
               printf("\t.long %#x\n", v);
     }
     printf("\n");

     printf("\t.global BOOTSIZE\n");
     printf("\t.p2align 2\n");
     printf("BOOTSIZE:\n");
     printf("\t.long %d\n\n", bp - mem);

     printf("\t.data\n");
     printf("\t.global MAIN\n");
     printf("\t.p2align 2\n");
     printf("MAIN:\n");
     printf("\t.long %d\n\n", find("main"));

     printf("\t.global UNKNOWN\n");
     printf("\t.p2align 2\n");
     printf("UNKNOWN:\n");
     printf("\t.long %d\n\n", find("unknown"));

     printf("\t.global dict\n");
     printf("\t.p2align 2\n");
     printf("dict:\n");
     printf("\t.long %d\n", dict);
}
