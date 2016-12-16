#!/bin/bash

if [ ! -f $CFAR_HOME/DieHard/src/libdiehard.so ]; then
	echo "ERROR: DieHard library not built/found" | tee warning.txt
	exit 1
fi

$PEASOUP_HOME/tools/update_env_var.sh DO_DIEHARD 1
cp $CFAR_HOME/DieHard/src/libdiehard.so libheaprand.so

