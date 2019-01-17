all: ninth

CC = gcc -m32
GLOBAL_CFLAGS = -Wall -O2 -fno-strict-aliasing
CFLAGS = $(GLOBAL_CFLAGS)

NINTH = kernel.o prims.o boot.o
ninth: $(NINTH)
	$(CC) $(CFLAGS) -o $@ $^

NINTH2 = kernel.o prims.o boot2.o
ninth2: $(NINTH2)
	$(CC) $(CFLAGS) -o $@ $^

kernel.o prims.o: CFLAGS = $(GLOBAL_CFLAGS) -DBOOT

boot.c: ninthboot system.nth
	ninthboot <system.nth | sed -f script
	[ -s $@ ]

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

boot2.s: ninthboot2 system.nth
	ninthboot2 <system.nth | sed -f script2
	[ -s $@ ]

NINTHBOOT = bootkernel.o bootprims.o init.o dump.o
ninthboot: $(NINTHBOOT)
	$(CC) $(CFLAGS) -o $@ $^

NINTHBOOT2 = bootkernel.o bootprims.o init.o dump2.o
ninthboot2: $(NINTHBOOT2)
	$(CC) $(CFLAGS) -o $@ $^

bootkernel.o bootprims.o init.o dump.o dump2.o: \
	CFLAGS = $(GLOBAL_CFLAGS) -DDUMP

boot%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: force
	rm -f boot.c ninth ninthboot ninth2 ninthboot2 tmpa \
		$(NINTHBOOT) $(NINTH) $(NINTH2)

force:

.DELETE_ON_ERROR:

###

$(NINTH) $(NINTHBOOT): ninth.h


