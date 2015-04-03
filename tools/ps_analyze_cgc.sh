#!/bin/sh 

# Default configuration for CGC Scored Event 2
#
# HLX :   Heap randomization
# SLX :   Stack padding
# SCFI:   Selective CFI
# IF  :   Input filtering
# SBX :   Sandbox crashing instructions (if sfuzz detects a crash)
#

export FIX_CALLS_FIX_ALL_CALLS=1

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step spawner=on 		\
	--step appfw=off 		\
	--step find_strings=off 	\
	--step preLoaded_ILR1=off	\
	--step preLoaded_ILR2=off	\
	--step sfuzz=on	\
	--step cinderella=on	\
	--step cgc_hlx=on	\
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
