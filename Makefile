all: ninth

NINTH = kernel.o prims.o boot.o main.o
ninth: $(NINTH)
	$(CC) $(CFLAGS) $^ -lm -o $@

NINTH2 = kernel.o prims.o boot2.o main.o
ninth2: $(NINTH2)
	$(CC) $(CFLAGS) $^ -lm -o $@

boot.c: ninthboot system.nth script
	ninthboot <system.nth >tmpa
	sed -f script tmpa
	test -s $@

boot.s: boot.c
	$(CC) $(CFLAGS) -S $<

boot2.s: ninthboot2 system.nth script2
	ninthboot2 <system.nth >tmpa
	sed -f script2 tmpa
	test -s $@

NINTHBOOT = bootkernel.o bootprims.o init.o dump.o
ninthboot: $(NINTHBOOT)
	$(CC) $(CFLAGS) $^ -lm -o $@

NINTHBOOT2 = bootkernel.o bootprims.o init.o dump2.o
ninthboot2: $(NINTHBOOT2)
	$(CC) $(CFLAGS) $^ -lm -o $@

init.o dump.o dump2.o: CFLAGS += -DINIT

boot%.o: %.c
	$(CC) $(CFLAGS) -DINIT -c $< -o $@

clean: force
	rm -f boot.c ninth ninthboot $(NINTH) $(NINTHBOOT)

force:

CC = gcc -m32
AS = gcc -m32 -c
CFLAGS = -g -Wall -fno-strict-aliasing

###

$(NINTH) $(NINTHBOOT): ninth.h
