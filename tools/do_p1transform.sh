#!/bin/sh
#
# do_p1transform.sh <originalBinary> <MEDS annotationFile> <cloneId> <BED_script>
#
# pre: we are in the top-level directory created by ps_analyze.sh
#

# input
CLONE_ID=$1
ORIGINAL_BINARY=$2
MEDS_ANNOTATION_FILE=$3
BED_SCRIPT=$4

echo "P1: transforming binary: cloneid=$CLONE_ID bed_script=$BED_SCRIPT"

# configuration variables
P1_DIR=p1.xform
EXECUTED_ADDRESSES=concolic.files_a.stratafied_0001/executed_address_list.txt
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt
BLACK_LIST=$P1_DIR/p1.filtered_out                                            # list of functions to blacklist
COVERAGE_FILE=$P1_DIR/p1.coverage

mkdir $P1_DIR

$PEASOUP_HOME/tools/cover.sh $ORIGINAL_BINARY $MEDS_ANNOTATION_FILE $EXECUTED_ADDRESSES $LIBC_FILTER $COVERAGE_FILE $BLACK_LIST
if [ $? -eq 0 ]; then
	if [ -f $COVERAGE_FILE ]; then
		echo "P1: issuing command: $SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe $cloneid $BLACK_LIST"
		$SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe $CLONE_ID $BED_SCRIPT $BLACK_LIST 
	else
		echo "No coverage file -- do not attempt P1 transform" > p1transform.out
		exit 1
	fi
else
	$SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe $CLONE_ID $BED_SCRIPT $LIBC_FILTER
fi

exit 0
