#!/bin/sh 

in=$1
out=$2

$PEASOUP_HOME/tools/ps_analyze.sh $* 	   	\
	--backend zipr	\
	--step c2e=on \

cgc2elf $2


# appfw was working?
