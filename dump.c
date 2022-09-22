// dump.c

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

     printf("// boot.c\n");
     printf("#define PORTABLE 1\n");
     printf("#include \"ninth.h\"\n\n");

     bp = ALIGN(bp, 4);
     dp = ALIGN(dp, 4);
     map_defs();

     printf("static const unsigned rom[] = {\n");
     n = k = 0; p = dmem;
     while (p < dp) {
          printf("/* %4d */ ", (p - dmem)/4);
          if (n < ndefs && p == (byte *) defs[n]) {
               def *d = (def *) p;
               printf("heading(%d, %u, %s), ",
                      d->d_next, d->d_flags, act_name[d->d_action]);

               if (k < nsyms && d->d_data == addrs[k])
                    printf("sym(%s),", syms[k]);
               else if (d->d_action == A_ENTER && (byte *) d->d_data >= dmem
			&& (byte *) d->d_data < dp) {
		    assert((unsigned) d->d_data % 4 == 0);
                    printf("sym(&rom[%d]),", ((byte *) d->d_data - dmem)/4);
	       } else {
                    printf("%#x,", (int) d->d_data);
	       }

               printf(" // %s\n", def_name(d));
               if (d == sdefs[k]) k++;
               p += sizeof(def); n++;
          } else {
               printf("%#x,\n", * (unsigned *) p);
               p += sizeof(unsigned);
          }
     }
     printf("};\n\n");

     n = 0;
     printf("const unsigned boot[] = {\n");
     for (unsigned *p = (unsigned *) mem; p < (unsigned *) bp; p++) {
          unsigned v = *p;

          if (n < ndefs && v == (unsigned) defs[n])
               printf("     sym(&rom[%d]), /* %s */\n",
                      (v - (unsigned) dmem)/4, def_name(defs[n++]));
          else
               printf("     ?%u?\n", v);
     }
     printf("};\n\n");

     printf("int dict = %d;\n", dict);
     printf("const unsigned BOOTSIZE = %d;\n", bp - mem);
     printf("const int MAIN = %d;\n", find("main"));
}
