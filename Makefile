all: ninth

NINTH = kernel.o prims.o boot.o main.o
ninth: $(NINTH)
	$(CC) $(CFLAGS) $^ -lm -o $@

boot.c: ninthboot system.nth script
	ninthboot <system.nth >tmpa
	sed -f script tmpa
	test -s $@

NINTHBOOT = bootkernel.o bootprims.o init.o dump.o
ninthboot: $(NINTHBOOT)
	$(CC) $(CFLAGS) $^ -lm -o $@

init.o dump.o: CFLAGS += -DINIT

boot%.o: %.c
	$(CC) $(CFLAGS) -DINIT -c $< -o $@

clean: force
	rm -f boot.c ninth ninthboot $(NINTH) $(NINTHBOOT)

force:

CC = gcc -m32
CFLAGS = -g -Wall -fno-strict-aliasing

###

$(NINTH) $(NINTHBOOT): ninth.h
