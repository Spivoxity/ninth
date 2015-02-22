all: ninth

NINTH = kernel.o
ninth: $(NINTH)
	gcc -o $@ $^
