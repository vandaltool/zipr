CC=gcc
LD=ld
CFLAGS= -c -Wall -Werror -fPIC
LFLAGS= -shared -ldl -fPIC

all: noh.so
noh.so: noh.o
	$(LD) $(LFLAGS) noh.o -o noh.so
noh.o: noh.c
	$(CC) $(CFLAGS) noh.c
test:
	make -C tests test
clean:
	rm *.so *.o
