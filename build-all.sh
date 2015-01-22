#!/bin/bash

# check if DIR is the directory containing the build script.
BUILD_LOC=`dirname $0`
FULL_BUILD_LOC=`cd $BUILD_LOC; pwd`
echo $PEASOUP_UMBRELLA_DIR
if [ "$PEASOUP_UMBRELLA_DIR" != "$FULL_BUILD_LOC" ]; then
    echo "PEASOUP_UMBRELLA_DIR differs from build-all.sh location ($FULL_BUILD_LOC).";
    echo "Did you source set_env_vars from the root of the umbrella working copy?";
    exit 1;
fi

# stratafier
cd $PEASOUP_UMBRELLA_DIR/stratafier
make

# strata
if [ ! "$STRATA_HOME" ]; then 
    echo "STRATA_HOME not set.";
    exit 1; 
fi

if [ `uname -m` = 'x86_64' ]; then
	# build 32-bit strata
	if [ ! -d $STRATA_HOME32 ] ; then 
		cd $STRATA
		make clean distclean
		cd $PEASOUP_UMBRELLA_DIR
		echo Creating strata 32-bit build directory
		cp -R $STRATA $STRATA32
	fi
	cd $STRATA_HOME32
	# mc2zk changed this line to ./build_cgc from ./build for cgc_dev
	STRATA_HOME=$STRATA_HOME32 STRATA=$STRATA_HOME32 ./build_cgc -host=i386-linux

	# build x86-64 strata
	cd $STRATA_HOME
	./configure;make

else
	cd $STRATA_HOME
	./build_cgc
fi

# smp-static-analyzer
if [ ! "$SMPSA_HOME" ]; then
    echo "SMPSA_HOME not set."; 
    exit 1; 
fi
cd $SMPSA_HOME
./configure
make

# security-transforms
if [ ! "$SECURITY_TRANSFORMS_HOME" ]; then 
    echo "SECURITY_TRANSFORMS_HOME not set."; 
    exit 1; 
fi
cd $SECURITY_TRANSFORMS_HOME
./configure --enable-cgc
./build.sh

cd $PEASOUP_HOME
make

