#!/bin/bash
#
# intxform_detect_benign_fp.sh <cloneId> <identifiedProgram> <outputFile>
#
# <cloneId>             variant id in the Peasoup database
# <identifiedProgram>   name of program identified via fingerprinting
#
# <outputFile>          contains list of addresses that triggered a benign error detection
#
# pre: we are in the top-level directory created by ps_analyze.sh
#

# input
CLONE_ID=$1
IDENTIFIED_PROG=$2

# output
INTEGER_WARNINGS_FILE=$3

# configuration variables
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt   # libc and other system library functions

TOP_DIR=`pwd`
INTEGER_ASPRI=a.irdb.integer.aspri
INTEGER_BSPRI=a.irdb.integer.bspri

touch $INTEGER_WARNINGS_FILE

echo "intxform(detect-benign-fp): transforming binary: cloneid=$CLONE_ID identifiedProg=$IDENTIFIED_PROG"

if [ "$BENIGN_FP_DETECT" != "1" ]; then
	echo "INTXFORM: Detection of benign false positives turned on for recognized program: $IDENTIFIED_PROG"
fi

echo "intxform(detect-benign-fp): Clone program"
$SECURITY_TRANSFORMS_HOME/libIRDB/test/clone.exe $CLONE_ID clone.id
tempcloneid=`cat clone.id`

#    - Transform program and run against all Grace-generated inputs using a policy of continued execution when an integer detector triggers (we want to catch all detection messages)
#    - Keep track of all inputs that trigger a C1 diagnostic and put in a list
echo "intxform(detect-benign-fp): Integer transform on cloned copy"
$SECURITY_TRANSFORMS_HOME/tools/transforms/integertransformdriver.exe $tempcloneid $LIBC_FILTER $INTEGER_WARNINGS_FILE --warning

# generate aspri, and assemble it to bspri
echo "intxform(detect-benign-fp): Generate temporary aspri --> bspri for integer transform"
$SECURITY_TRANSFORMS_HOME/libIRDB/test/generate_spri.exe $($PEASOUP_HOME/tools/is_so.sh a.ncexe) $tempcloneid $INTEGER_ASPRI
$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $INTEGER_ASPRI $INTEGER_BSPRI stratafier.o.exe libstrata.so.symbols

#if [ $? -eq 0 ]; then
# produce list of instruction addresses that trigger an integer detector
#	echo "intxform(detect-benign-fp): false positives detection activated"
#		timeout $TIMEOUT $PEASOUP_HOME/tools/integer_replay.sh $TOP_DIR/a.stratafied $CONCOLIC_DIR $TOP_DIR/$INTEGER_BSPRI $INTEGER_WARNINGS_FILE
#		sort $INTEGER_WARNINGS_FILE | uniq > $INTEGER_WARNINGS_FILE.$$
#		mv $INTEGER_WARNINGS_FILE.$$ $INTEGER_WARNINGS_FILE
#
#		cd $TOP_DIR   # restore working dir (just in case)
#	else
#		echo "Error generating integer transforms -- skip replay step to detect benign false positives"
#	fi
#fi

$NUM_FP_DETECTED=`wc -l $INTEGER_WARNINGS_FILE`
echo "------------ intxform: end detection of benign false positives: $NUM_FP_DETECTED benign false positives detected -----------------"

