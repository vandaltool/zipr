#!/bin/bash 

infile=$1
addr=$2
outfile=$3

gcc $infile -nostdlib -Wl,--section-start -Wl,eh_frame_hdr=$addr -Wl,-e -Wl,0x1000  -Wl,--build-id=none -Wl,-T -Wl,${PEASOUP_HOME}/tools/eh_frame_tools/eh_frame.ls -o $outfile -Wl,-Map,$outfile.map -static -no-pie -fno-PIC || exit
#eu-readelf -S ./a.out
objcopy --rename-section eh_frame_hdr=.eh_frame_hdr --rename-section eh_frame=.eh_frame --rename-section gcc_except_table=.gcc_except_table $outfile
#eu-readelf -S -w ./b.out

