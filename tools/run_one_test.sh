#!/bin/sh

# usage: run_one_test.sh <testSubDir> <bSpri>
# Given a test subdirectory, figure out whether the test succeeded or failed by comparing outputs

# layout
#
# TEST_DIR/spec/input              'contains specified inputs
# TEST_DIR/spec/output             'contains specified outputs
# TEST_DIR/transformed/output      'contains output obtained by running the user-specified command

TEST_DIR=$1
BSPRI=$2

SPEC_DIR=$TEST_DIR/spec
XFORMED_DIR=$TEST_DIR/transformed

echo "running test $TEST_DIR from $XFORMED_DIR using bspri file: $BSPRI"
cd $XFORMED_DIR
./test_new_cmd.sh $BSPRI
status=$?

if [ $status -eq 139 ]; then
  exit $status 
else
  echo "test command status code: $status"
fi

cd output

# make sure we have same number of files in the output directory
num_files_orig=`ls $SPEC_DIR/output | wc -l`
num_files_xformed=`ls | wc -l`

if [ "$num_files_orig" != "$num_files_xformed" ]; then
  echo "Test $TEST_DIR failed -- different number of output files detected"
  exit 1
fi

for i in `ls`
do
  diff $i $SPEC_DIR/output/$i
  if [ ! $? -eq 0 ]; then
    echo "Test $1 failed -- $i output differ from the original at $SPEC_DIR/output/$i"
    echo "Content original ($i):"
    cat $SPEC_DIR/output/$i
    echo "Content transformed ($i):"
    cat $i

    exit 1
  fi
done

echo "Test $1 passed"
exit 0
