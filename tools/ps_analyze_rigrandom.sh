#!/bin/bash 

in=$1
out=$2
random_char=$3

# chop off the last argument before passing args to ps_analyze
length=$(($#-1))
argv=${@:1:$length}

$PEASOUP_HOME/tools/ps_analyze.sh $argv 	   	\
	--backend zipr	\
	--step rigrandom=on \
	--step-option $random_char \
	--step gather_libraries=off \
