#!/bin/bash
######################################################################
######################################################################
# This file is used as a template, not actually for running the code #
######################################################################
######################################################################

#
# determine the directory that contains the files for peasoup
#
datapath=$1

#
# save original $0
#

origbinpath=$2

#
# grab the rest of the args in $*
#
shift 2;

#
# Run the program with the proper env. vars set., and the arguments to the program specified
#



command="
LD_PRELOAD=$datapath/libappfw.so
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$datapath
APPFW_DB=$datapath/appfw.db
APPFW_SIGNATURE_FILE=$datapath/a.ncexe.sigs
STRATA_WATCHDOG=0
STRATA_NUM_HANDLE=0
STRATA_DOUBLE_FREE=0
STRATA_HEAPRAND=0
STRATA_CONTROLLED_EXIT=0
STRATA_PC_CONFINE=0
STRATA_PC_CONFINE_XOR=0				
STRATA_REKEY_AFTER=5000
STRATA_PC_CONFINE_XOR_KEY_LENGTH=1024		
STRATA_ANNOT_FILE=$datapath/a.ncexe.annot 
STRATA_IS_SO=0
STRATA_SIEVE=1					
STRATA_RC=1					
STRATA_PARTIAL_INLINING=0			
STRATA_EXE_FILE=$datapath/a.stratafied
STRATA_MAX_WARNINGS=500000
	exec -a $origbinpath $datapath/a.stratafied \"\$@\""


#
# setup signitures for appfw
#
cp $datapath/a.ncexe.sigs.orig $datapath/a.ncexe.sigs
if [ ! -g a.ncexe -a ! -u a.ncexe ]; then
	echo $datapath/a.stratafied >> $datapath/a.ncexe.sigs
	echo $origbinpath >> $datapath/a.ncexe.sigs
	echo $PWD >> $datapath/a.ncexe.sigs
	for var in "$@"
	do
		# Split whitespace in arguments and add to sigs
		echo "$var" | tr ' ' '\n' | grep -v '^[<spc><tab>]*$' >> $datapath/a.ncexe.sigs
	done

fi


#
#  If STRATA_LOG is clear, no additional logging was requested, and we just always need to log to a file.
#
if [ -z $STRATA_LOG ]; then
	command="STRATA_LOG=detectors STRATA_OUTPUT_FILE=$datapath/diagnostics.out $command"
else
	# otherwise, we add detectors to the strata_log list, and log to stderr	
	command="STRATA_LOG=$STRATA_LOG,detectors $command"
fi

#
# Set mmap files location if preLoaded_ILR is on
#
if [ -f $datapath/a.stratafied.data_hash.ini ]; then
	command="STRATA_SPRI_MMAP_DIR=$datapath/ $command"
fi

#
# Set SPRI file to use (should be generated from the IRDB).
#
# check for faster versions of the spri file first
if [ -f $datapath/a.irdb.fbspri  -o $datapath/a.irdb.fbspri.reloc ]; then
	command="STRATA_SPRI_FILE=$datapath/a.irdb.fbspri $command"
# default to the slow one
elif [ -f $datapath/a.irdb.bspri ]; then
	command="STRATA_SPRI_FILE=$datapath/a.irdb.bspri $command"
fi


if [ ! -z $VERBOSE ]; then
	echo $command
fi

eval $command
