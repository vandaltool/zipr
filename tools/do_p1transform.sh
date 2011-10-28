#!/bin/bash
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
TIMEOUT_VALUE=$5

# configuration variables
P1_DIR=p1.xform
EXECUTED_ADDRESSES=concolic.files_a.stratafied_0001/executed_address_list.txt
EXECUTED_ADDRESSES_MANUAL=manual_coverage.txt
EXECUTED_ADDRESSES_FINAL=final.coverage.txt
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt
BLACK_LIST=$P1_DIR/p1.filtered_out                                            # list of functions to blacklist
COVERAGE_FILE=$P1_DIR/p1.coverage

PN_BINARY=$SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe

echo "P1: transforming binary: cloneid=$CLONE_ID bed_script=$BED_SCRIPT timeout_value=$TIMEOUT_VALUE"

execute_pn()
{
	echo "P1: issuing command: $SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe $1 $2 $3 with timeout value=$TIMEOUT_VALUE"

	# On timeout send SIGUSR1
	timeout -10 $TIMEOUT_VALUE $PN_BINARY $1 $2 $3
}

mkdir $P1_DIR

# generate coverage info for manually-specified tests (if any)
$PEASOUP_HOME/tools/do_manual_cover.sh

# merge all execution traces
touch $EXECUTED_ADDRESSES_FINAL
cat $EXECUTED_ADDRESSES_MANUAL >> $EXECUTED_ADDRESSES_FINAL
cat $EXECUTED_ADDRESSES >> $EXECUTED_ADDRESSES_FINAL

$PEASOUP_HOME/tools/cover.sh $ORIGINAL_BINARY $MEDS_ANNOTATION_FILE $EXECUTED_ADDRESSES_FINAL $LIBC_FILTER $COVERAGE_FILE $BLACK_LIST

if [ $? -eq 0 ]; then
	if [ -f $COVERAGE_FILE ]; then
		execute_pn $CLONE_ID $BED_SCRIPT $BLACK_LIST $TIMEOUT_VALUE
	else
		echo "No coverage file -- do not attempt P1 transform" > p1transform.out
		exit 1
	fi
else
	execute_pn $CLONE_ID $BED_SCRIPT $LIBC_FILTER $TIMEOUT_VALUE
fi

exit 0
