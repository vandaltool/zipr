#!/bin/sh

#
# BED: Behavioral Equivalence Detector
#
# Usage: bed_manual.sh <variantID> <ASPRI_file> <BSPRI_file>
#
# Use user-specified input/output pair 
# Assume we are in the Peasoup sub-directory created by ps_analyze.sh
#


PEASOUP_DIR=`pwd`
MANUAL_TEST_DIR=$PEASOUP_DIR/manual_tests

variantid=$1
aspri=$2
bspri=$3

# generate the bspri code
$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $aspri $bspri $PEASOUP_DIR/stratafier.o.exe  > spasm.out 2> spasm.err
if [ ! $? -eq 0 ]; then
  echo "BED: spasm error -- exiting"
  exit 1
fi

ls $MANUAL_TEST_DIR/* >/dev/null 2>/dev/null
if [ ! $? -eq 0 ]; then
  echo "BED: error: no manual test specifications found -- exiting"
  exit 1
fi

#for i in `ls | grep "test.*" | grep -v ".sh"`
for i in `ls $MANUAL_TEST_DIR`
do
  echo "running test with cmd: $PEASOUP_HOME/tools/run_one_test.sh $MANUAL_TEST_DIR/$i $bspri"
  $PEASOUP_HOME/tools/run_one_test.sh $MANUAL_TEST_DIR/$i $bspri
  status=$?
  if [ ! $status -eq 0 ]; then
    echo "BED says: test $i failed"
    exit $status
  else
    echo "BED says: test $i passed"
  fi
done

echo "BED says: all tests passed: copyping $bspri into $PEASOUP_DIR"

# copy bspri file
cp $bspri $PEASOUP_DIR

exit 0
