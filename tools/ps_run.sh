#!/bin/sh

#
# determine the directory that contains the files for peasoup
#
datapath=$1

#
# grab the rest of the args in $*
#
shift;

#
# Determine which SPRI file to use;  first priorty goes to one generated from the IRDB.
#
if [ -f $datapath/a.irdb.bspri ]; then
	export STRATA_SPRI_FILE=$datapath/a.irdb.bspri
elif [ -f $datapath/p1.xform/p1.final.bspri ]; then
	export STRATA_SPRI_FILE=$datapath/p1.xform/p1.final.bspri 	
fi

#
# Run the program with the proper env. vars set., and the arguments to the program specified
#

STRATA_DOUBLE_FREE=1 					\
	STRATA_HEAPRAND=1 				\
	STRATA_PC_CONFINE=1 				\
	STRATA_PC_CONFINE_XOR=1				\
	STRATA_PC_CONFINE_XOR_KEY_LENGTH=1024		\
	STRATA_ANNOT_FILE=$datapath/a.ncexe.annot 	\
	$datapath/a.stratafied "$@"


