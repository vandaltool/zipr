#!/bin/sh

make clean
./configure CPPFLAGS="-fno-stack-protector"
echo 
echo "Making $1"
echo
make $1
echo
echo "--------------------BEGIN PS-------------------------------"
echo "PS_analyzing $1"
echo
$PEASOUP_HOME/tools/ps_analyze.sh $1 $1.dynamic.protected

echo "--------------------END PS-------------------------------"

