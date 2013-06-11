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
TOP_DIR=`pwd`
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt   # libc and other system library functions
ORIG_BINARY=a.ncexe
INTEGER_ASPRI=a.irdb.integer.aspri
INTEGER_BSPRI=a.irdb.integer.bspri
REGRESSION_TESTS=$PEASOUP_HOME/tests/$IDENTIFIED_PROG/test_script.sh

touch $INTEGER_WARNINGS_FILE

echo "intxform(detect-benign-fp): transforming binary: cloneid=$CLONE_ID identifiedProg=$IDENTIFIED_PROG"

if [ -f $REGRESSION_TESTS ]; then
	echo "intxform(detect-benign-fp): manual regression tests detected for $IDENTIFIED_PROG"
else
	echo "intxform(detect-benign-fp): no manual regression tests detected for $IDENTIFIED_PROG"
	exit 1
fi

echo "intxform(detect-benign-fp): Clone program"
$SECURITY_TRANSFORMS_HOME/libIRDB/test/clone.exe $CLONE_ID clone.id
tempcloneid=`cat clone.id`

echo "intxform(detect-benign-fp): Integer transform on cloned copy"
$SECURITY_TRANSFORMS_HOME/tools/transforms/integertransformdriver.exe $tempcloneid $LIBC_FILTER $INTEGER_WARNINGS_FILE --warning

# generate aspri, and assemble it to bspri
echo "intxform(detect-benign-fp): Generate temporary aspri --> bspri for integer transform"
$SECURITY_TRANSFORMS_HOME/libIRDB/test/generate_spri.exe $($PEASOUP_HOME/tools/is_so.sh a.ncexe) $tempcloneid $INTEGER_ASPRI
$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $INTEGER_ASPRI $INTEGER_BSPRI stratafier.o.exe libstrata.so.symbols

# generate script to run instrumented binary
DETECTOR_BINARY=benignfp.detector
$PEASOUP_HOME/tools/intxform_make_detector_binary.sh $DETECTOR_BINARY

# run regression tests
rm -f $TOP_DIR/diagnostics.cumul.out
touch $TOP_DIR/diagnostics.cumul.out
$PEASOUP_HOME/tools/intxform_replay.sh $REGRESSION_TESTS $TOP_DIR/$DETECTOR_BINARY $TOP_DIR/$ORIG_BINARY $TOP_DIR/$INTEGER_BSPRI $TOP_DIR/diagnostics.cumul.out $INTEGER_WARNINGS_FILE

NUM_FP_DETECTED=`wc -l $INTEGER_WARNINGS_FILE`
echo "------------ intxform: end detection of benign false positives: $NUM_FP_DETECTED benign false positives detected -----------------"
