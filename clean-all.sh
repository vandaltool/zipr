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

# clean main strata
cd $STRATA_HOME
./configure
make clean
rm Makefile

# clean strata32 if exists.
cd $PEASOUP_UMBRELLA_DIR/
rm -Rf strata32

cd $SMPSA_HOME
scons -c || exit

cd $SECURITY_TRANSFORMS_HOME
scons -c || exit

cd $IRDB_TRANSFORMS
scons -c || exit

if [ -d $ZIPR_SCFI_PLUGIN ]; then
	cd $ZIPR_SCFI_PLUGIN
	scons  -c || exit
fi

cd $PEASOUP_UMBRELLA_DIR/zipr_large_only_plugin/
scons -c || exit

cd $PEASOUP_UMBRELLA_DIR/zipr_push64_reloc_plugin
scons -c || exit

if [ -d $ZIPR_HOME ]; then 
	cd $ZIPR_HOME
	scons -c || exit
fi

cd $PEASOUP_UMBRELLA_DIR
./clean_diehard.sh
