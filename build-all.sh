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

if [ `basename $FULL_BUILD_LOC` == "cfar_umbrella" ]; then
	cfar_mode="--enable-cfar"
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
		if [ -f Makefile ] ; then
			make clean distclean
		fi
		cd $PEASOUP_UMBRELLA_DIR
		echo Creating strata 32-bit build directory
		cp -R $STRATA $STRATA32
	fi
	
	cd $STRATA_HOME32
	STRATA_HOME=$STRATA_HOME32 STRATA=$STRATA_HOME32 ./build -host=i386-linux $cfar_mode || exit

	# build x86-64 strata
	cd $STRATA_HOME
	if [ -f Makefile -a Makefile -nt configure -a Makefile -nt Makefile.in ]; then
		echo Skipping Strata reconfigure step
	else
		./configure $cfar_mode || exit
	fi
	make || exit

else
	cd $STRATA_HOME
	./build $cfar_mode || exit
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

if [ -d $ZIPR_CALLBACKS ]; then 
	cd $ZIPR_CALLBACKS
	./configure --enable-p1 --prefix=$ZIPR_INSTALL
	make  || exit
	make install || exit
fi

if [ -d $ZIPR_HOME ]; then
	cd $ZIPR_HOME
	scons  || exit
fi

if [ -d $ZIPR_SCFI_PLUGIN ]; then
	cd $ZIPR_SCFI_PLUGIN
	scons  || exit
fi

cd $PEASOUP_UMBRELLA_DIR/zipr_large_only_plugin/
scons || exit

cd $PEASOUP_UMBRELLA_DIR/zipr_push64_reloc_plugin
scons || exit

cd $IRDB_TRANSFORMS
scons || exit


cd $PEASOUP_UMBRELLA_DIR
if [ -d DieHard ]; then
	./build_diehard.sh
fi

cd $PEASOUP_UMBRELLA_DIR


echo
echo
echo  "Overall build complete."
echo
echo
