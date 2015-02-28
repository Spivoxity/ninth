all: ninth

CC = gcc
GLOBAL_CFLAGS = -Wall -O2 -fno-strict-aliasing
CFLAGS = $(GLOBAL_CFLAGS)

NINTH = kernel.o prims.o
ninth: $(NINTH)
	$(CC) $(CFLAGS) -o $@ $^

kernel.o prims.o: CFLAGS = $(GLOBAL_CFLAGS) -DBOOT

boot.c: ninthboot system.nth
	ninthboot <system.nth | sed -f script
	[ -s $@ ]

NINTHBOOT = bootkernel.o bootprims.o init.o dump.o
ninthboot: $(NINTHBOOT)
	$(CC) $(CFLAGS) -o $@ $^

bootkernel.o bootprims.o init.o dump.o: CFLAGS = $(GLOBAL_CFLAGS) -DDUMP

boot%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: force
	rm -f boot.c ninth ninthboot tmpa $(NINTHBOOT) $(NINTH)

force:

.DELETE_ON_ERROR:

###

$(NINTH) $(NINTHBOOT): ninth.h
kernel.o: boot.c

