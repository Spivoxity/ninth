all: ninth

CC = gcc

NINTH = kernel.o
ninth: $(NINTH)
	$(CC) -o $@ $^
