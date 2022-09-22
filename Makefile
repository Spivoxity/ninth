include config.make

all: ninth ninth2

NINTH = kernel.o prims.o boot.o main.o
ninth: $(NINTH)
	$(CC) $(CFLAGS) $^ -lm -o $@

NINTH2 = kernel.o prims.o boot2.o main.o
ninth2: $(NINTH2)
	$(CC) $(CFLAGS) $^ -lm -o $@

boot.c: ninthboot system.nth script
	./ninthboot <system.nth >tmpa
	sed -n -f script tmpa
	test -s $@

%.s: %.c
	$(CC) $(CFLAGS) -S $<

boot2.s: ninthboot2 system.nth script2
	./ninthboot2 <system.nth >tmpa
	sed -f script2 tmpa
	test -s $@

boot2.o: boot2.s
	as $(HOSTASFLAGS) -defsym PORTABLE=1 $< -o $@

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
	rm -f boot.c ninth ninthboot $(NINTH) $(NINTHBOOT) tmpa
	rm -f boot2.s ninth2 ninthboot2 $(NINTH2) $(NINTHBOOT2)

force:

CC = gcc
CFLAGS = -O2 -Wall -fno-strict-aliasing $(HOSTCFLAGS)

###

$(NINTH) $(NINTHBOOT) $(NINTH2) $(NINTHBOOT2): ninth.h
