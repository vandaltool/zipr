#!/bin/bash 

infile=$1
addr=$2
outfile=$3

# Try gcc with -no-pie option, but some don't support it
# so then try normal gcc
# and if both fail, then we exit with an error code

$PS_GCC $infile -nostdlib -Wl,--section-start -Wl,eh_frame_hdr=$addr -Wl,-e -Wl,0x1000  -Wl,--build-id=none -Wl,-T -Wl,${PEASOUP_HOME}/tools/eh_frame_tools/eh_frame.ls -o $outfile -Wl,-Map,$outfile.map -static -no-pie -fno-PIC  || \
$PS_GCC $infile -nostdlib -Wl,--section-start -Wl,eh_frame_hdr=$addr -Wl,-e -Wl,0x1000  -Wl,--build-id=none -Wl,-T -Wl,${PEASOUP_HOME}/tools/eh_frame_tools/eh_frame.ls -o $outfile -Wl,-Map,$outfile.map -static  || \
exit 1
#eu-readelf -S ./a.out
$PS_OBJCOPY --rename-section eh_frame_hdr=.eh_frame_hdr --rename-section eh_frame=.eh_frame --rename-section gcc_except_table=.gcc_except_table $outfile
#eu-readelf -S -w ./b.out

