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
make || exit

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
	STRATA_HOME=$STRATA_HOME32 STRATA=$STRATA_HOME32 ./build -host=i386-linux || exit

	# build x86-64 strata
	cd $STRATA_HOME
	./configure || exit
	make || exit

else
	cd $STRATA_HOME
	./build || exit
fi

# smp-static-analyzer
if [ ! "$SMPSA_HOME" ]; then
    echo "SMPSA_HOME not set."; 
    exit 1; 
fi

# security-transforms
if [ ! "$SECURITY_TRANSFORMS_HOME" ]; then 
    echo "SECURITY_TRANSFORMS_HOME not set."; 
    exit 1; 
fi

cd $SECURITY_TRANSFORMS_HOME
scons || exit

cd $SMPSA_HOME
scons  || exit

cd $PEASOUP_HOME
make || exit

cd $ZIPR_CALLBACKS
./configure --enable-p1 --prefix=$ZIPR_INSTALL
make  || exit
make install || exit


if [ -d $ZIPR_HOME ]; 
	cd $ZIPR_HOME
	scons  || exit
fi

if [ -d $ZIPR_SCFI_PLUGIN ];
then
	cd $ZIPR_SCFI_PLUGIN
	scons  || exit
fi

cd $PEASOUP_UMBRELLA_DIR/zipr_push64_reloc_plugin
scons || exit

cd $IRDB_TRANSFORMS
scons || exit
