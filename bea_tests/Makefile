

INCLUDES= -I ../include -I../beaengine/include
LIBS= -L ../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d
OPT=-g

.SUFFIXES: .exe .c .o .bin .s .dis .beadis .o .nasm .diff
.PRECIOUS: %.exe %.o %.bin %.s %.dis %.beadis %.o %.nasm %.diff

diffs=3dnow.diff insns_mm.diff insns_nomm_no3dnow.diff insns.diff avx1.diff avx2.diff
BIN=dbea.exe

all: dbea.exe  $(diffs)


$(BIN): dbea.c bea
	gcc $(INCLUDES)  $(OPT) $< $(LIBS) -o $@



%.bin: %.nasm 
	nasm -O0 -fbin -o$@ $<

%.dis: %.bin 
	ndisasm -b64 $<  > $@


%.o: %.s
	gcc -c $< -o $@

%.bin: %.o
	objcopy -O binary $< $@ 

%.beadis: %.bin dbea.exe
	./dbea.exe < $< > $@

%.diff: %.dis %.beadis
	sdiff -w 134 -W $^  > $@  || true

test: $(diffs)
	
	
	

bea:
	cd ..; make bea

clean:
	rm -f *.bin *.dis *.beadis dbea.exe *.o 3dnow.s insns_mm.s insns_nomm_no3dnow.s *.diff


#
# note: insns.s was manually extracted from ffmpeg's disassembly.
#
# this is how these were made.
#
3dnow.s:
	cat insns.s |grep pavgusb> $@

insns_mm.s:
	cat insns.s |grep -v pavgusb |grep mm > $@

insns_nomm_no3dnow.s:
	cat insns.s |grep -v pavgusb |grep -v mm > $@
