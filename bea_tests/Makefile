

INCLUDES= -I ../include -I../beaengine/include
LIBS= -L ../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d
OPT=-g

width=130

.SUFFIXES: .exe .c .o .bin .s .dis .beadis .o .nasm .diff .rebin .redis .rediff .cmp .recmp .cmpdebug .recmpdebug .trimdis 

diffs=3dnow.diff insns_mm.diff insns_nomm_no3dnow.diff insns.diff avx1.diff avx2.diff
BIN=dbea.exe

all: dbea.exe  $(diffs)


$(BIN): dbea.c bea
	gcc $(INCLUDES)  $(OPT) $< $(LIBS) -o $@



%.bin: %.nasm 
	nasm -O0 -fbin -o$@ $<

%.dis: %.bin 
	${HOME}/nasm*/bin/ndisasm -b64 $<  > $@


%.o: %.s
	gcc -c $< -o $@

%.bin: %.o
	objcopy -O binary $< $@ 

%.beadis: %.bin dbea.exe
	./dbea.exe < $< -pretty -pretty > $@

%.rebin: %.dis
	echo "bits 64" > tmp
	cat $< |sed "s/  */ /g"|cut -d" " -f3- >> tmp; done
	nasm -O0 -fbin -o$@ tmp
%.redis: %.rebin 
	${HOME}/nasm*/bin/ndisasm -b64 $<  > $@

%.trimredis: %.redis
	cat $^ |sed -e "s/  */	/g" -e "s/byte +0x33/0x33/" | cut -d"	" -f2- > $@


%.rebeabin: %.beadis
	cat $< |sed "s/  */ /g"|cut -d" " -f3- > tmp; done
	nasm -O1 -fbin -o$@ tmp

%.rebeadis: %.rebeabin 
	${HOME}/nasm*/bin/ndisasm -b64 $<  > $@

%.trimrebeadis: %.rebeadis
	cat $^ |sed -e "s/  */	/g" -e "s/byte +0x33/0x33/" |cut -d"	" -f2- > $@

%.trimbeadis: %.beadis
	cat $^ |sed -e "s/  */	/g" -e "s/byte +0x33/0x33/" |cut -d"	" -f2- > $@

%.trimdis: %.dis
	cat $^ |sed "s/  */	/g"|cut -d"	" -f2- > $@


%.recmp: %.rebeabin %.rebin 
	cmp  $^  > $@  || true

%.recmpdebug: %.trimredis %.trimrebeadis
	echo $^ > $@
	sdiff -w ${width} -W $^  >> $@  || true

%.cmpdebug: %.trimdis %.trimbeadis
	echo $^ > $@
	sdiff -w ${width} -W $^  >> $@  || true

%.rediff: %.recmp %.recmpdebug
	cat $^ > $@

%.diff: %.cmpdebug
	cat $^ > $@

test: avx1.diff avx1.rediff
	
	
	

bea:
	cd ..; make bea

clean:
	rm -f *.bin *.dis *.beadis dbea.exe *.o 3dnow.s insns_mm.s insns_nomm_no3dnow.s *.diff *.trim*  *.cmp* *.recmp*


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
