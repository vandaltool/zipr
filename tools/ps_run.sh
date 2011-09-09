#!/bin/sh
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
# grab the rest of the args in $*
#
shift;

#
# Set SPRI file to use (should be generated from the IRDB).
#
if [ -f $datapath/a.irdb.bspri ]; then
	export STRATA_SPRI_FILE=$datapath/a.irdb.bspri
fi

#
# Run the program with the proper env. vars set., and the arguments to the program specified
#


command="
STRATA_DOUBLE_FREE=0
STRATA_HEAPRAND=0
STRATA_PC_CONFINE=0
STRATA_PC_CONFINE_XOR=0				
STRATA_PC_CONFINE_XOR_KEY_LENGTH=1024		
STRATA_ANNOT_FILE=$datapath/a.ncexe.annot 
STRATA_SIEVE=1					
STRATA_RC=1					
STRATA_PARTIAL_INLINING=0			
STRATA_LOG=detectors				
STRATA_OUTPUT_FILE=$datapath/diagnostics.out	
	$datapath/a.stratafied"

if [ ! -z $VERBOSE ]; then
	echo $command
fi

eval $command "$@"

SAVE_EXIT_CODE=$?

if [ ! -z $datapath/diagnostics.out ]; then
	cat $datapath/diagnostics.out
fi

exit $SAVE_EXIT_CODE
