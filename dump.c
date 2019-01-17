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

     printf("const unsigned boot[%d] = {\n", (dp - mem)/4);

     while (p < dp) {
          printf("/* %4d */ ", p - mem);

          if (n >= 0 && p == (uchar *) defs[n]) {
               def *d = (def *) p;
               printf("heading(%d, %u, %u), ", 
                      d->d_next, d->d_flags, d->d_action);

               if (k < nsyms && d == sdefs[k] && d->d_data == addrs[k])
                    printf("sym(&%s),", syms[k]);
               else if (d->d_data >= mem && d->d_data < dp)
                    printf("sym(&mem[%d]),", d->d_data - mem);
               else if (d->d_data >= tmem && d->d_data < tp)
                    printf("sym(&rom[%d]),", (d->d_data - tmem)/2);
               else
                    printf("%d,", (int) d->d_data);

               if (d == sdefs[k]) k++;

               printf(" /* %s */\n", def_name(d));
               p += sizeof(def); n--;
          }
          else {
               printf("%u,\n", * (unsigned *) p);
               p += sizeof(unsigned);
          }
     }

     printf("};\n\n");
}

void dump_rom(void) {
     short *p = (short *) tmem;

     printf("#define ROM %d\n\n", (tp - tmem)/2);
     printf("const short rom[ROM] = {\n");

     while (p < (short *) tp) {
          int n = 0;
          printf("/* %4d */", p - (short *) tmem); 
          while (n++ < 8 && p < (short *) tp)
               printf(" %d,", *p++);
          printf("\n");
     }

     printf("};\n\n");
}

void dump(void) {
     printf("// --boot--\n");
     printf("#include \"ninth.h\"\n");
     map_defs();
     dump_rom();
     dump_mem();
     printf("const unsigned BOOTSIZE = %d;\n", dp - mem);
     printf("const ushort DICT = %d;\n", dict);
     printf("const ushort MAIN = %d;\n", tok(find("main")));
}
