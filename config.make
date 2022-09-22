# config.make

ARCH := $(shell uname -m)

CFLAGS-x86_64 = -m32 -nopie
ASFLAGS-x86_64 = --32

HOSTCFLAGS = $(CFLAGS-$(ARCH))
HOSTASFLAGS = $(CFLAGS-$(ARCH))
