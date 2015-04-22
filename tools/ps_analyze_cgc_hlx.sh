#!/bin/sh 

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step spawner=on 		\
	--step appfw=off 		\
	--step find_strings=off 	\
	--step preLoaded_ILR1=off	\
	--step preLoaded_ILR2=off	\
	--step cinderella=on	\
	--step cgc_hlx=on	\
	--step watch_allocate=off	\
	--step integertransform=off	\
	--step p1transform=off	\
	--step fast_spri=off	\
	--step fast_annot=off	\
	--step zipr=on	\

