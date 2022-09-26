// ninth/main.c
// Copyright (C) 2022 J. M. Spivey

#define EXTERN
#include "ninth.h"

byte mem[MEMSIZE];
byte *rstack[RSTACK];
char inbuf[INBUF];
char pad[PAD];

// underflow -- report stack underflow
void underflow(void) {
     printf("\nStack underflow!\n");
}

int main(int argc, char **argv) {
     memcpy(mem, boot, BOOTSIZE);
     dp = &mem[BOOTSIZE];
     sbase = &mem[MEMSIZE - 4];
     rbase = &rstack[RSTACK];
     args = argv;
     run(MAIN);
     return 0;
}
