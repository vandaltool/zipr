#!/bin/bash

#
# Default configuration for CFE:
#  - SCFI
#

# for selective_cfi, turn on env. var
SCFI=off
case "$*" in 
   *selective_cfi=on* ) 
	export FIX_CALLS_FIX_ALL_CALLS=1
	echo "SCFI is on -- turn on FIX_CALLS_FIX_ALL_CALLS=1"
	SCFI=on
	;;
esac

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step spawner=off 		\
	--step appfw=off 		\
	--step find_strings=off 	\
	--step preLoaded_ILR1=off	\
	--step preLoaded_ILR2=off	\
	--step sfuzz=off	\
	--step cinderella=off	\
	--step cgc_hlx=off	\
	--step-option cgc_hlx:--do_allocate_padding=4096 \
	--step-option cgc_hlx:--shr_malloc_factor=5 \
	--step-option cgc_hlx:--do_malloc_padding=32 \
	--step heaprand=off	\
	--step double_free=off	\
	--step controlled_exit=off	\
	--step detect_server=off	\
	--step watchdog=off	\
	--step signconv_func_monitor=off	\
	--step rekey=off	\
	--step p1transform=off	\
	--step-option p1transform:--min_stack_padding=64 \
	--step-option p1transform:--max_stack_padding=64 \
	--step-option p1transform:--recursive_min_stack_padding=32 \
	--step-option p1transform:--recursive_max_stack_padding=32 \
	--step-option p1transform:--canaries=off \
	--step-option p1transform:--should_double_frame_size=false \
	--step input_filtering=off	\
	--step watch_allocate=off	\
	--step integertransform=off	\
	--step selective_cfi=$SCFI	\
	--step fast_spri=off	\
	--step fast_annot=off	\
	--step spasm=off	\
	--step ilr=off	\
	--backend zipr
