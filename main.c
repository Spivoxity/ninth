// main.c

#include "ninth.h"

int main(int argc, char **argv) {
     printf("BOOTSIZE = %d\n", BOOTSIZE);
     memcpy(mem, boot, BOOTSIZE);
     dp = &mem[BOOTSIZE];
     args = argv;
     run(MAIN);
     return 0;
}
