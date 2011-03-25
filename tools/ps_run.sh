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
# Run the program with the proper env. vars set., and the arguments to the program specified
#
STRATA_DOUBLE_FREE=1 					\
	STRATA_HEAPRAND=1 				\
	STRATA_PC_CONFINE=1 				\
	STRATA_PC_CONFINE_XOR=1				\
	STRATA_PC_CONFINE_XOR_KEY_LENGTH=1024		\
	STRATA_ANNOT_FILE=$datapath/a.ncexe.annot 	\
#	STRATA_SPRI_FILE=$datapath/p1.xform/p1.final.bspri 	\
	$datapath/a.stratafied $*


