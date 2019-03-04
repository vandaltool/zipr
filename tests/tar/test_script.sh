#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for grep, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/tar
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=tar

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

run_basic_test 120 --help
run_basic_test 120 --version
run_basic_test 120 --doesnotexist
run_basic_test 120 -cvf $DATA_DIR.tar $DATA_DIR
run_basic_test 120 -tvf $DATA_DIR/test.tar $DATA_DIR

# sanity check tar functionality
timeout 10 $BENCH -tf $DATA_DIR.tar | grep dir1
if [ ! $? -eq 0 ];then 
	report_failure
fi

rm $DATA_DIR.tar
cleanup

report_success
