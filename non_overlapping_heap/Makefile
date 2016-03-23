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
	LD_PRELOAD="./noh.so" ls
	LD_PRELOAD="./noh.so ../DieHard/src/libheaprand.so" ls
clean:
	rm *.so *.o
