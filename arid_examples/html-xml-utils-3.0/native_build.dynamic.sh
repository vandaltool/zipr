#!/bin/sh

make clean
./configure CPPFLAGS="-fno-stack-protector"
echo Making $1
make $1
echo


