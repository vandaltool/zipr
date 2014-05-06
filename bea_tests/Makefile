

INCLUDES= -I ../include -I../beaengine/include
LIBS= -L ../beaengine/lib/Linux.gnu.Debug -lBeaEngine_s_d
OPT=-g

width=180


finals= 3dnow.rebeabin insns_mm.rebeabin insns_nomm_no3dnow.rebeabin insns_avx.rebeabin avx1.rebeabin avx2.rebeabin \
3dnow.diff insns_mm.diff insns_nomm_no3dnow.diff insns_avx.diff avx1.diff avx2.diff \
3dnow.halfdiff insns_mm.halfdiff insns_nomm_no3dnow.halfdiff insns_avx.halfdiff avx1.halfdiff avx2.halfdiff \

.SUFFIXES: .exe .c .o .bin .s .dis .beadis .o .nasm .diff .rebin .redis .rediff .cmp .recmp .cmpdebug .recmpdebug .trimdis 
.PRECIOUS: %.exe %.c %.o %.bin %.s %.dis %.beadis %.o %.nasm %.diff %.rebin %.redis %.rediff %.cmp %.recmp %.cmpdebug %.recmpdebug %.trimdis 

diffs=3dnow.diff insns_mm.diff insns_nomm_no3dnow.diff insns.diff avx1.diff avx2.diff
BIN=dbea.exe

#NDISASM=${HOME}/nasm*/bin/ndisasm 
NDISASM=ndisasm

all: dbea.exe  $(finals) 


$(BIN): dbea.c bea
	gcc $(INCLUDES)  $(OPT) $< $(LIBS) -o $@



%.bin: %.nasm 
	nasm -O0 -fbin -o$@ $<

%.dis: %.bin 
	${NDISASM} -b64 $<  > $@


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
	${NDISASM} -b64 $<  > $@

%.trimredis: %.redis
	cat $^ |sed -e "s/  */	/g"  | cut -d"	" -f2- > $@


%.rebeabin: %.beadis
	cat $< |sed "s/  */ /g"|cut -d" " -f3- > tmp; done
	nasm -w-all -O1 -fbin -o$@ tmp

%.rebeadis: %.rebeabin 
	${NDISASM} -b64 $<  > $@

%.trimrebeadis: %.rebeadis
	#cat $^ |sed -e "s/  */	/g" -e "s/byte	0x33/0x33/" |cut -d"	" -f2- > $@
	cat $^ |sed -e  "s/  */	/g" |cut -d"	" -f2- > $@

%.trimbeadis: %.beadis
	cat $^ |sed -e  "s/  */	/g" |cut -d"	" -f2- > $@
	#cat $^ |sed -e "s/  */	/g" -e "s/byte	0x/0x/" -e "s/.word//" -e "s/yword//" |cut -d"	" -f2- > $@

%.trimdis: %.dis
	#cat $^ |sed -e "s/  */	/g" -e "s/byte	0x/0x/" -e "s/.word//" -e "s/yword//" |cut -d"	" -f2- > $@
	cat $^ |sed -e  "s/  */	/g" |cut -d"	" -f2- > $@


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

%.halfdiff: %.trimdis %.trimrebeadis
	echo $^ > $@
	sdiff -w ${width} $^ >> $@ || true

test: avx1.diff avx1.rediff
	
	
	

bea:
	cd ..; make bea

clean:
	rm -f *.bin *.dis *.beadis dbea.exe *.o 3dnow.s insns_mm.s insns_nomm_no3dnow.s *.diff *.trim*  *.cmp* *.recmp* *.rebin *.rediff *.redis tmp *.rebeadis *.halfdiff *.trimrebeadis *.rebeadis *.rebeabin


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

insns_avx.s: insns_mm.s
	cat $< |grep ^v  > $@
