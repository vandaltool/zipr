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
make clean

# strata
if [ ! "$STRATA_HOME" ]; then 
    echo "STRATA_HOME not set.";
    exit 1; 
fi
cd $STRATA_HOME
./configure
make clean
cd $PEASOUP_UMBRELLA_DIR/
rm -Rf strata32

# smp-static-analyzer
if [ ! "$SMPSA_HOME" ]; then
    echo "SMPSA_HOME not set."; 
    exit 1; 
fi
cd $SMPSA_HOME
scons -c

# security-transforms
if [ ! "$SECURITY_TRANSFORMS_HOME" ]; then 
    echo "SECURITY_TRANSFORMS_HOME not set."; 
    exit 1; 
fi
cd $SECURITY_TRANSFORMS_HOME
scons build_cgc=1 build_appfw=0 -c

cd $PEASOUP_HOME
make clean

# clean Cinderella callbacks
cd $ZIPR_CALLBACKS
./configure_for_cinderella --prefix=$ZIPR_INSTALL
make clean

if [ -d $ZIPR_HOME ]; then
        cd $ZIPR_HOME
        scons build_cgc=1 -c 
fi

if [ -d $ZIPR_SCFI_PLUGIN ]; then
        cd $ZIPR_SCFI_PLUGIN
        scons do_cgc=1 -c
fi

# clean trace plugin
if [ -d "${PEASOUP_UMBRELLA_DIR}/zipr_trace_plugin" ]; then
	cd ${PEASOUP_UMBRELLA_DIR}/zipr_trace_plugin/libtrace
	scons -c
	cd ${PEASOUP_UMBRELLA_DIR}/zipr_trace_plugin
	scons do_cgc=1 -c
fi

# clean unpin plugin
if [ -d "${PEASOUP_UMBRELLA_DIR}/zipr_unpin_plugin" ]; then
	cd ${PEASOUP_UMBRELLA_DIR}/zipr_unpin_plugin
	scons do_cgc=1 -c
fi

# clean daffy
cd $DAFFY_HOME
make clean

