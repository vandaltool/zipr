#!/bin/sh

#
# BED: Behavioral Equivalence Detector
#
# Usage: bed_manual.sh <variantID> <ASPRI_file> <BSPRI_file>
#
# Use user-specified input/output pair 
# 
# Assumptions:
#    (1) we are in the Peasoup sub-directory created by ps_analyze.sh
#    (2) there exists a file "new_command_name" whose content is the basename of the peasoupified target binary
#

PEASOUP_DIR=`pwd`

variantid=$1
aspri=$2
bspri=$3

SCRIPT_NAME=$PEASOUP_DIR/manual_test_wrapper

# generate the bspri code
$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $aspri $bspri $PEASOUP_DIR/stratafier.o.exe  > spasm.out 2> spasm.err
if [ ! $? -eq 0 ]; then
  echo "BED: spasm error -- exiting"
  exit 1
fi

# toolchain expects a BSPRI file at this location
cp $bspri $PEASOUP_DIR/a.irdb.bspri

NAME=`cat new_command_name`
TRANSFORMED_PROGRAM="$PEASOUP_DIR/$NAME"

# invoke the user-supplied test script
# pass as argument the transformed program
echo "blackbox BED: Invoking: $SCRIPT_NAME $TRANSFORMED_PROGRAM"
eval $SCRIPT_NAME $TRANSFORMED_PROGRAM
status=$? 

cd $PEASOUP_DIR

echo "blackbox BED: Done with: $SCRIPT_NAME $TRANSFORMED_PROGRAM: status: $status"
exit $status
