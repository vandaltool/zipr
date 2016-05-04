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

# security-transforms
if [ ! "$SECURITY_TRANSFORMS_HOME" ]; then 
    echo "SECURITY_TRANSFORMS_HOME not set."; 
    exit 1; 
fi



cd $SECURITY_TRANSFORMS_HOME
scons -j2 build_cgc=1 build_appfw=0

cd $SMPSA_HOME
scons -j2

cd $PEASOUP_HOME
make -j2

# build Cinderella callbacks
cd $ZIPR_CALLBACKS
./configure_for_cinderella --prefix=$ZIPR_INSTALL
make
make install_cinderella

# build regular callbacks
cd $ZIPR_CALLBACKS
./configure_for_cgc --prefix=$ZIPR_INSTALL
make clean all
make install

if [ -d $ZIPR_HOME ]; then
	cd $ZIPR_HOME
	scons -j2 build_cgc=1 # ./configure --enable-cgc --prefix=$ZIPR_INSTALL; make;  make install
fi

if [ -d $ZIPR_SCFI_PLUGIN ]; then 
	cd $ZIPR_SCFI_PLUGIN
	scons -j2 do_cgc=1
fi

# build trace plugin
if [ -d "${PEASOUP_UMBRELLA_DIR}/zipr_trace_plugin" ]; then
	cd ${PEASOUP_UMBRELLA_DIR}/zipr_trace_plugin/libtrace
	scons
	cd ${PEASOUP_UMBRELLA_DIR}/zipr_trace_plugin
	scons do_cgc=1
	cp ${PEASOUP_UMBRELLA_DIR}/zipr_trace_plugin/libtrace/libtrace.so ${ZIPR_INSTALL}/lib
fi

# build unpin plugin
if [ -d "${PEASOUP_UMBRELLA_DIR}/zipr_unpin_plugin" ]; then
	cd ${PEASOUP_UMBRELLA_DIR}/zipr_unpin_plugin
	scons do_cgc=1
fi

# build daffy (32 bit only)
uname -a | grep i686
if [ $? -eq 0 ]; then
	if [ -d "$DAFFY_HOME" ]; then
		cd $DAFFY_HOME
		make
	fi
fi

