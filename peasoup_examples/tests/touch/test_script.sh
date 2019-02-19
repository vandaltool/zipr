#!/bin/bash
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#used for filtering program names from output.
ORIG_NAME=touch

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

# sanity check
timeout 10 $BENCH --help | grep FILE
if [ ! $? -eq 0 ];then
	report_failure
fi

run_basic_test 20 --help
run_basic_test 20 --version
rm -f tmp
run_basic_test 20 "tmp"
rm -f tmp
#no arg test
run_basic_test 10

report_success
