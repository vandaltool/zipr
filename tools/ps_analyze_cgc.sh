#!/bin/sh 

#
# Default configuration for CGC Scored Event 2
#
# HLX :   Heap padding (malloc_padding=size<<5 + 64 bytes, allocate_padding=4096 bytes)
# SLX :   Stack padding (64 bytes)
# SCFI:   Selective CFI (indirect branches)
# IF  :   Input filtering (64 bytes max at a time for receive())
# SBX :   Sandbox crashing instructions (only if sfuzz detects a crash)
#

export FIX_CALLS_FIX_ALL_CALLS=1

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step spawner=off 		\
	--step appfw=off 		\
	--step find_strings=off 	\
	--step preLoaded_ILR1=off	\
	--step preLoaded_ILR2=off	\
	--step sfuzz=on	\
	--step cinderella=on	\
	--step cgc_hlx=on	\
	--step-option cgc_hlx:--do_malloc_padding=64 \
	--step-option cgc_hlx:--shr_malloc_factor=5 \
	--step-option cgc_hlx:--do_allocate_padding=4096 \
	--step heaprand=off	\
	--step double_free=off	\
	--step controlled_exit=off	\
	--step detect_server=off	\
	--step watchdog=off	\
	--step signconv_func_monitor=off	\
	--step rekey=off	\
	--step p1transform=on	\
	--step input_filtering=on	\
	--step watch_allocate=on	\
	--step integertransform=off	\
	--step selective_cfi=on	\
	--step fast_spri=off	\
	--step fast_annot=off	\
	--step spasm=off	\
	--step ilr=off	\
	--step zipr=on	\
