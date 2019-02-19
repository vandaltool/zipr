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
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$datapath
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
STRATA_MAX_WARNINGS=50000
	exec -a $origbinpath $datapath/a.stratafied \"\$@\""

command="STRATA_LOG=detectors STRATA_OUTPUT_FILE=$datapath/diagnostics.out $command"

# make sure we pick up the BSPRI file genreated by intxform when it's trying to detect
# benign false positives
command="STRATA_SPRI_FILE=$datapath/a.irdb.integer.bspri $command"

eval $command
