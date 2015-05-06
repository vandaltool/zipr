#!/bin/bash


if [ `uname -s` = 'SunOS' ]; then
# SunOS == solaris == different compiler by default.
	export CC="cc -I/opt/csw/include -L /opt/csw/lib/ -g"
	export CXX="CC -I/opt/csw/include -L /opt/csw/lib/ -g"
fi

#sanity check
if [ $SECURITY_TRANSFORMS_HOME"X" = "X" ]; then
        echo Please set SECURITY_TRANSFORMS_HOME properly
        exit
fi

# make everything
cd $SECURITY_TRANSFORMS_HOME
echo "Build transformer library + associated tools"
make all -f Makefile
