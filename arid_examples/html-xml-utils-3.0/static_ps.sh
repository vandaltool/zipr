#!/bin/sh

make clean
./configure CPPFLAGS="-fno-stack-protector" LDFLAGS="-static"
echo 
echo Making $1
echo 
make $1
echo "-------------BEGIN PS $1.static ---------------"
echo PS_analyzing $1
echo
$PEASOUP_HOME/tools/ps_analyze.sh $1 $1.static.protected
echo "-------------END PS $1.static ---------------"


