#!/bin/bash

if [ ! -f $CFAR_HOME/DieHard/src/libdiehard-4k-x64.so ]; then
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

file a.ncexe |grep -q "64-bit"

if  (file a.ncexe |grep -q "64-bit") ; then 
	echo "Detected 64-bit binary" 
	ext=x64
else 
	echo "Detected 32-bit binary" 
	ext=x32
fi

if [ -z "$seq" ]; then
	cp $CFAR_HOME/DieHard/src/libdiehard-32k-$ext.so libheaprand.so
else
	if [ ! -f $CFAR_HOME/DieHard/src/libdiehard-4k-$ext.so ]; then
		echo "ERROR: DieHard library 4k not built/found" | tee warning.txt
		exit 1
	fi

	if [ ! -f $CFAR_HOME/DieHard/src/libdiehard-32k-$ext.so ]; then
		echo "ERROR: DieHard library 32k not built/found" | tee warning.txt
		exit 1
	fi

	if [ $(expr ${seq} % 2) = 0 ]; then
		cp $CFAR_HOME/DieHard/src/libdiehard-32k-$ext.so libheaprand.so
	else
		cp $CFAR_HOME/DieHard/src/libdiehard-4k-$ext.so libheaprand.so
	fi
fi
