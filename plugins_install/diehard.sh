#!/bin/bash

if [ ! -f $CFAR_HOME/DieHard/src/libdiehard.so ]; then
	echo "ERROR: DieHard library not built/found" | tee warning.txt
	exit 1
fi

$PEASOUP_HOME/tools/update_env_var.sh DO_DIEHARD 1

seq=""
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
	--structured_heap)
		seq="$2"
		shift
		shift
	;;
	*)
		shift
	;;
esac
done

if [ -z "$seq" ]; then
	cp $CFAR_HOME/DieHard/src/libdiehard.so libheaprand.so
else
	if [ ! -f $CFAR_HOME/DieHard/src/libdiehard-4k.so ]; then
		echo "ERROR: DieHard library 4k not built/found" | tee warning.txt
		exit 1
	fi

	if [ ! -f $CFAR_HOME/DieHard/src/libdiehard-32k.so ]; then
		echo "ERROR: DieHard library 32k not built/found" | tee warning.txt
		exit 1
	fi

	if [ $(expr ${seq} % 2) = 0 ]; then
		cp $CFAR_HOME/DieHard/src/libdiehard-32k.so libheaprand.so
	else
		cp $CFAR_HOME/DieHard/src/libdiehard-4k.so libheaprand.so
	fi
fi
