# ninth/config.make
# Copyright (C) 2022 J. M. Spivey

ARCH := $(shell uname -m)

CFLAGS-x86_64 = -m32 -no-pie
ASFLAGS-x86_64 = --32

HOSTCFLAGS = $(CFLAGS-$(ARCH))
HOSTASFLAGS = $(ASFLAGS-$(ARCH))
