#!/bin/sh

make clean
./configure CPPFLAGS="-fno-stack-protector" LDFLAGS="-static"
echo Making $1
echo
make $1


