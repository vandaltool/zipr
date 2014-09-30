#!/bin/sh 

$PEASOUP_HOME/tools/ps_analyze.sh $* 	\
	--step spawner=on 		\
	--step appfw=off 		\
	--step find_strings=off 		

