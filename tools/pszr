#!/bin/bash 

source $(dirname $0)/ps_wrapper.source $0

meds_static_opt=" -s meds_static=off"
rida_opt=" -s rida=on"

echo $@ | grep "meds_static=" >/dev/null 2>&1
if [ $? -eq 0 ]; then
	meds_static_specified=1
	meds_static_opt=" "
fi

echo $@ | grep "rida=" >/dev/null 2>&1
if [ $? -eq 0 ]; then
	rida_specified=1
	rida_opt=" "
fi

$PEASOUP_HOME/tools/ps_analyze.sh "$@" --backend zipr $meds_static_opt $rida_opt
