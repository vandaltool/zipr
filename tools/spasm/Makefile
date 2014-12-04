CC=g++
CFLAGS= -Wall -g# -DUBUNTU -Wall -O3

INCLUDE=-I. -I../../include -I../../xform 
LIBS=-L../../xform -lxform 

.cpp.o .c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

all: spasm
	echo spasm build complete

clean:
	rm -f *.o core spasm *.map *.bspri *.asm *.bin

spasm: $(OBJS) spasm.cpp Makefile spasm.h spasm_main.cpp ben_lib.cpp ben_lib.h ../../xform/libxform.a 
	$(CC) -o spasm $(INCLUDE) $(CFLAGS) spasm_main.cpp  spasm.cpp ben_lib.cpp $(OBJS) $(LIBS)
