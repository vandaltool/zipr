#!/bin/bash

#
# Default configuration for CGC with Zipr only
#
#
# NOTE: The LD_PRELOAD below -- this may cause 
# problems but I (Will) need it on the VMs to
# readdir() etc from NFS.
#

cd /techx_share/techx_umbrella/peasoup/
source set_env_vars
cd -
export LD_PRELOAD=/usr/local/lib/inode64.so

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step spawner=off 		\
	--step appfw=off 		\
	--step find_strings=off 	\
	--step preLoaded_ILR1=off	\
	--step preLoaded_ILR2=off	\
	--step heaprand=off	\
	--step double_free=off	\
	--step controlled_exit=off	\
	--step detect_server=off	\
	--step watchdog=off	\
	--step signconv_func_monitor=off	\
	--step rekey=off	\
	--step input_filtering=off	\
	--step integertransform=off	\
	--step fast_spri=off	\
	--step fast_annot=off	\
	--step spasm=off	\
	--step ilr=off	\
	--step zipr=on	\
	--backend=zipr
