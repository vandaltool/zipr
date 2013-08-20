#!/bin/sh

#sanity check
if [ $SECURITY_TRANSFORMS_HOME"X" = "X" ]; then
        echo Please set SECURITY_TRANSFORMS_HOME properly
        exit
fi

cd $SECURITY_TRANSFORMS_HOME

# build ELF utility library
#echo "Build ELFIO library"
#cd $SECURITY_TRANSFORMS_HOME/ELFIO-1.0.3
#./configure --prefix=$SECURITY_TRANSFORMS_HOME

# build dissassembler library -- makefile takes care of this now.
#echo "Build BEA Engine library"
#cd $SECURITY_TRANSFORMS_HOME/beaengine
#cmake .
# 

# make everything
cd $SECURITY_TRANSFORMS_HOME
echo "Build transformer library + associated tools"
make all -f Makefile
