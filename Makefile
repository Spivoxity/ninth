# ninth/Makefile
# Copyright (C) 2022 J. M. Spivey

include config.make

all: ninth-arm.elf

NINTH = kernel.o prims.o boot.o main.o
ninth: $(NINTH)
	$(CC) $(CFLAGS) $^ -lm -o $@

NINTH-ARM = native-a.o prims-a.o boot-a.o main-a.o
ninth-arm.elf: $(NINTH-ARM)
	$(ARMCC) $(ARMCFLAGS) $^ -lm -o $@

%-a.o: %.c
	$(ARMCC) $(ARMCFLAGS) -c $< -o $@

%-a.o: %.s
	$(ARMCC) $(ARMCFLAGS) -c $< -o $@

boot.s: ninthboot system.nth script
	./ninthboot <system.nth >tmpa
	sed -n -f script tmpa
	test -s $@

boot.o: boot.s
	as $(HOSTASFLAGS) -defsym PORTABLE=1 $< -o $@

NINTHBOOT = kernel.o bootprims.o initprims.o init.o dump.o
ninthboot: $(NINTHBOOT)
	$(CC) $(CFLAGS) $^ -lm -o $@

boot%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: force
	rm -f boot.s ninth ninthboot $(NINTH) $(NINTHBOOT)
	rm -f ninth-arm.elf $(NINTH-ARM) tmpa

force:

CC = gcc
CFLAGS = -O2 -Wall -fno-strict-aliasing $(HOSTCFLAGS)

ARMCC = arm-linux-gnueabihf-gcc
ARMCFLAGS = -O2 -Wall -fno-strict-aliasing

###

$(NINTH) $(NINTHBOOT): ninth.h
