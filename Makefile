all: ninth

CC = gcc
CFLAGS = -O2 -fno-strict-aliasing

ninth: kernel.c boot.c
	$(CC) $(CFLAGS) -DBOOT -o $@ $<

boot.c: ninthboot system.nth
	ninthboot <system.nth | tee tmpa
	sed  '1,/--boot/d' tmpa >$@
	[ ! -z $@ ]

ninthboot: kernel.c
	$(CC) $(CFLAGS) -DDUMP -o $@ $<
