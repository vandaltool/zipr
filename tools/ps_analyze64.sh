#!/bin/sh 

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step meds_static=off 		\
	--step concolic=off 		\
	--step p1transform=off 		\
	--step integertransform=off 	\
	--step preLoaded_ILR1=off 	\
	--step preLoaded_ILR2=off 	\
	--step fast_annot=off 		\
	--step ibtc=off 		\
	--step sieve=off 		\
	--step return_cache=off	 	\
	--step partial_inlining=off 	\
	--step watchdog=off 		\
	--step signconv_func_monitor=off\
	--step rekey=off	 	\


