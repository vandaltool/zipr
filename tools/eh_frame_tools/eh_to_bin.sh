#!/bin/bash 

gcc sample.eh.s -nostdlib -Wl,--section-start -Wl,eh_frame_hdr=0x602000 -Wl,-e -Wl,0x1000  -Wl,--build-id=none -Wl,-T -Wl,./eh_frame.ls || exit
eu-readelf -S ./a.out
objcopy --rename-section eh_frame_hdr=.eh_frame_hdr --rename-section eh_frame=.eh_frame --rename-section gcc_except_table=.gcc_except_table a.out b.out
eu-readelf -S -w ./b.out

