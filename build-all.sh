#!/bin/bash



if [[ "$*" =~ "--debug" ]]; then
	SCONSDEBUG=" debug=1 "
fi


if [[ $(uname -m) == 'armv7l' ]]; then
	scons $SCONSDEBUG 
else 
	scons $SCONSDEBUG -j 3
fi

exit



# check if DIR is the directory containing the build script.
BUILD_LOC=`dirname $0`
FULL_BUILD_LOC=`cd $BUILD_LOC; pwd`
echo $PEASOUP_UMBRELLA_DIR
if [ "$PEASOUP_UMBRELLA_DIR" != "$FULL_BUILD_LOC" ]; then
    echo "PEASOUP_UMBRELLA_DIR differs from build-all.sh location ($FULL_BUILD_LOC).";
    echo "Did you source set_env_vars from the root of the umbrella working copy?";
    exit 1;
fi

echo
echo "Building Zipr toolchain."
echo


SCONSDEBUG=""
if [[ "$*" =~ "--debug" ]]; then
	SCONSDEBUG=" debug=1 "
fi

if [ `basename $FULL_BUILD_LOC` == "cfar_umbrella" ]; then
	cfar_mode="--enable-cfar"
fi

mkdir -p $ZEST_RUNTIME/lib32
mkdir -p $ZEST_RUNTIME/lib64
mkdir -p $ZEST_RUNTIME/bin
mkdir -p $ZEST_RUNTIME/sbin

if [ "$(ls -A "$PEDI_HOME" 2> /dev/null)" == "" ]; then
	echo "pedi submodule is empty. Did you clone using --recursive?";
	exit 1;
fi 

if [ ! -f manifest.txt.config -o ! -d "$PS_INSTALL" ]; then
	mkdir -p "$PS_INSTALL"
	$PEDI_HOME/pedi --setup -m manifest.txt -l ida -l ida_key -l ps -l zipr -l stars -i $PS_INSTALL
fi


use_strata=0
if [[ $use_strata = 1 ]]; then
	# stratafier
	cd $PEASOUP_UMBRELLA_DIR/stratafier
	make || exit 1

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
		STRATA_HOME=$STRATA_HOME32 STRATA=$STRATA_HOME32 ./build -host=i386-linux || exit 1

		# build x86-64 strata
		cd $STRATA_HOME
		if [ -f Makefile -a Makefile -nt configure -a Makefile -nt Makefile.in ]; then
			echo Skipping Strata reconfigure step
		else
			./configure $cfar_mode || exit 1
		fi
		make || exit 1

	else
		cd $STRATA_HOME
		./build $cfar_mode || exit 1
	fi
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
scons $SCONSDEBUG -j 3 || exit 1

cd $SMPSA_HOME
scons $SCONSDEBUG -j 3 || exit 1

cd $SECURITY_TRANSFORMS_HOME
scons build_deep=1 $SCONSDEBUG -j 3 || exit 1

cd $PEASOUP_HOME
make || exit 1

if [ -d $ZIPR_CALLBACKS ]; then 
	cd $ZIPR_CALLBACKS
	./configure --enable-p1 --prefix=$ZIPR_INSTALL
	make  || exit 1
	make install || exit 1
fi

if [ -d $ZIPR_HOME ]; then
	cd $ZIPR_HOME
	scons $SCONSDEBUG -j 3|| exit 1
fi

if [ -d $ZIPR_SCFI_PLUGIN ]; then
	cd $ZIPR_SCFI_PLUGIN
	scons  $SCONSDEBUG || exit 1
fi

if [ -d $ZIPR_XEON_PLUGIN ]; then
        cd $ZIPR_XEON_PLUGIN
        scons  $SCONSDEBUG || exit
fi

cd $PEASOUP_UMBRELLA_DIR/zipr_large_only_plugin/
scons $SCONSDEBUG || exit 1

if [[ -e $PEASOUP_UMBRELLA_DIR/zipr ]] && [[ -e $PEASOUP_UMBRELLA_DIR/zipr_relax_plugin ]]  ; then
	cd $PEASOUP_UMBRELLA_DIR/zipr_relax_plugin/
	scons $SCONSDEBUG || exit 1
fi

if [[ -e $PEASOUP_UMBRELLA_DIR/zipr ]] && [[ -e $PEASOUP_UMBRELLA_DIR/zipr_trace_plugin ]]  ; then
	cd $PEASOUP_UMBRELLA_DIR/zipr_trace_plugin/
	scons $SCONSDEBUG || exit 1
fi

cd $PEASOUP_UMBRELLA_DIR/zipr_push64_reloc_plugin
scons $SCONSDEBUG || exit 1

cd $PEASOUP_UMBRELLA_DIR/zipr_unpin_plugin
scons $SCONSDEBUG || exit 1

cd $IRDB_TRANSFORMS
scons $SCONSDEBUG -j 3 || exit 1

if [[ -d "$DAFFY_HOME" ]] && [[ $(uname -p) == 'x86_64' ]]; then
	cd $DAFFY_HOME
	./setup_cfar.sh || exit 1
fi

cd $PEASOUP_UMBRELLA_DIR

$PEDI_HOME/pedi -m manifest.txt 

echo
echo
echo  "Zipr toolchain build complete."
echo
echo
