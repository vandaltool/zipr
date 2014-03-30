

INCLUDES= -I ../include -I../beaengine/include
LIBS= -L ../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d
OPT=-g

.SUFFIXES: .exe .c .o

.c.exe:  
	gcc $(INCLUDES)  $(OPT) $^ $(LIBS) -o $@

all: bea dbea.exe avx1.bin avx2.bin

dbea.exe: dbea.c ../beaengine/lib/Linux.gnu.Debug/libBeaEngine_s_d.a
	gcc $(INCLUDES)  $(OPT) $< $(LIBS) -o $@

avx1.bin: avx.s
	nasm -O0 -fbin -oavx1.bin avx.s

avx2.bin: avx.s
	nasm -Ox -fbin -oavx2.bin avx.s

avx1.dis avx2.dis: 	avx1.bin avx2.bin
	ndisasm -b64 avx1.bin  > avx1.dis
	ndisasm -b64 avx2.bin  > avx2.dis

test: bea dbea.exe avx1.bin avx2.bin avx1.dis avx2.dis
	./dbea.exe<avx1.bin > avx1.beadis
	./dbea.exe<avx2.bin > avx2.beadis
	sdiff avx1.dis avx1.beadis|less
	
test2: bea dbea.exe avx1.bin avx2.bin avx1.dis avx2.dis
	./dbea.exe<avx1.bin > avx1.beadis
	./dbea.exe<avx2.bin > avx2.beadis
	sdiff avx2.dis avx1.beadis|less
	

bea:
	cd ..; make bea

clean:
	rm -f *.bin *.dis *.beadis dbea.exe
