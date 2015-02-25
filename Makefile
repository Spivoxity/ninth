all: ninth

CC = gcc
CFLAGS = -O2 -fno-strict-aliasing

ninth: kernel.c boot.c
	$(CC) $(CFLAGS) -DBOOT -o $@ $<

boot.c: ninthboot system.nth
	ninthboot <system.nth | sed -f script
	[ ! -z $@ ]

ninthboot: kernel.c
	$(CC) $(CFLAGS) -DDUMP -o $@ $<

clean: force
	rm -f boot.c ninth ninthboot tmpa

force:
