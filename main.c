// main.c

#include "ninth.h"

int main(int argc, char **argv) {
     memcpy(mem, boot, BOOTSIZE);
     dp = &mem[BOOTSIZE];
     sbase = &mem[MEMSIZE - 4];
     rbase = &rstack[RSTACK];
     args = argv;
     printf("Hello\n");
     run(MAIN);
     return 0;
}
