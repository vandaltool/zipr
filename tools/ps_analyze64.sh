#!/bin/sh 

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step concolic=off 		\
	--step integertransform=off 	\
	--step ibtc=off 		\
	--step sieve=off 		\
	--step partial_inlining=off 	\
	--step return_cache=off	 	\

#	--step rekey=off	 	\
#	--step p1transform=off 		\
#	--step fast_annot=off 		\
#	--step meds_static=off 		\
#	--step signconv_func_monitor=off\
#	--step watchdog=off 		\

