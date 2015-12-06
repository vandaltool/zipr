#!/bin/bash 


if [ ! -f DieHard/src/libdiehard.so ]; then

	sed -i "s|/dev/urandom|/dev/cfar_urandom|" \
		DieHard/src/archipelago/benchmarks/pine4.64/imap/src/osdep/unix/ssl_unix.c \
		DieHard/src/include/rng/realrandomvalue.h \
		DieHard/src/exterminator/src/realrandomvalue.h 
	cd DieHard/src
	make linux-gcc-x86-64
fi

echo Diehard Build complete.

