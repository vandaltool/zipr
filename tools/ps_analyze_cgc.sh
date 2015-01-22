#!/bin/sh 

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step spawner=on 		\
	--step appfw=off 		\
	--step find_strings=off 	\
	--step preLoaded_ILR1=off	\
	--step preLoaded_ILR2=off	\

