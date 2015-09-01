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
# not on solaris
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
	STRATA_HOME=$STRATA STRATA=$STRATA ./build -host=i386-linux

	# build x86-64 strata
	cd $STRATA_HOME
	./configure;make

else
	cd $STRATA_HOME
	#./build
	./configure
	make
fi


# security-transforms
if [ ! "$SECURITY_TRANSFORMS_HOME" ]; then 
    echo "SECURITY_TRANSFORMS_HOME not set."; 
    exit 1; 
fi
cd $SECURITY_TRANSFORMS_HOME
scons  build_appfw=0

cd $PEASOUP_HOME
make

