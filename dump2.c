/* dump.c */

#include "ninth.h"

#define MAXDEFS 200

def *defs[MAXDEFS];
int ndefs = 0;

void map_defs(void) {
     int d = dict;

     while (d >= 0) {
          defs[ndefs++] = def(d);
          d = def(d)->d_next;
     }
}

void dump_mem(void) {
     uchar *p = mem;
     int n = ndefs-1;
     int k = 0;

     printf("\t.data\n");
     printf("boot:\n");

     while (p < dp) {
          if (n >= 0 && p == (uchar *) defs[n]) {
               def *d = (def *) p;
               printf("\t.short %d  /* %s */\n", d->d_next, def_name(d));
               printf("\t.byte %u, %u\n", d->d_flags, d->d_action);

               if (k < nsyms && d == sdefs[k] && d->d_data == addrs[k])
                    printf("\t.long %s\n", syms[k]);
               else if (d->d_data >= mem && d->d_data < dp)
                    printf("\t.long mem+%d\n", d->d_data - mem);
               else if (d->d_data >= tmem && d->d_data < tp)
                    printf("\t.long rom+%d\n", d->d_data - tmem);
               else
                    printf("\t.long %d\n", (int) d->d_data);

               if (d == sdefs[k]) k++;
               p += sizeof(def); n--;
          }
          else {
               printf("\t.long %u\n", * (unsigned *) p);
               p += sizeof(unsigned);
          }
     }
     printf("bootend:\n\n");
}

void dump_rom(void) {
     short *p = (short *) tmem;

     printf("\t.data\n");
     printf("rom:\n");

     while (p < (short *) tp) {
          int n = 1;
          printf("\t.short %d", *p++);
          while (n++ < 8 && p < (short *) tp)
               printf(", %d", *p++);
          printf("\n");
     }

     printf("\n");
}

void dump(void) {
     printf("/* --boot-- */\n");
     map_defs();
     printf("\t.global boot, BOOTSIZE, DICT, MAIN\n\n");
     dump_rom();
     dump_mem();
     printf("\t.data\n");
     printf("BOOTSIZE:\n\t.long bootend-boot\n");
     printf("DICT:\n\t.short %u\n", dict);
     printf("MAIN:\n\t.short %u\n", tok(find("main")));
}
