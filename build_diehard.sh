#!/bin/bash 


if [ ! -f DieHard/src/libdiehard.so ]; then

	sed -i "s|/dev/urandom|/dev/cfar_urandom|" \
		DieHard/src/archipelago/benchmarks/pine4.64/imap/src/osdep/unix/ssl_unix.c \
		DieHard/src/include/rng/realrandomvalue.h \
		DieHard/src/exterminator/src/realrandomvalue.h 
	cd DieHard/src
	g++ -std=c++11 -W -Wall -O0 -pipe -fPIC -m64 -march=nocona -ffast-math -g -I. -Iinclude -Iinclude/layers -Iinclude/util -Iinclude/math -Iinclude/static -Iinclude/rng -Iinclude/hoard -Iinclude/superblocks -IHeap-Layers -DDIEHARD_DIEHARDER=0 -D_REENTRANT=1 -DDIEHARD_MULTITHREADED=1 -shared  -D'CUSTOM_PREFIX(x)=diehard##x' Heap-Layers/wrappers/gnuwrapper.cpp source/libdieharder.cpp -Bsymbolic -o libdiehard.so -ldl -lpthread -Dlinux
fi

echo Diehard Build complete.

