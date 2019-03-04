#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for grep, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/readelf
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=readelf

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

# sanity check readelf
timeout 10 $BENCH -h /bin/ls | grep -i "header" >/dev/null 2>&1
if [ ! $? -eq 0 ]; then
	report_failure
fi

run_basic_test 120 --help
run_basic_test 120 --version
run_basic_test 120 --doesnotexist
run_basic_test 120 -V 
run_basic_test 120 -a /bin/ls
run_basic_test 120 -g /bin/ls
run_basic_test 120 -t /bin/ls
run_basic_test 120 -e /bin/ls
run_basic_test 120 -s /bin/ls
run_basic_test 120 -s --dyn-syms /bin/ls
run_basic_test 120 -n /bin/ls
run_basic_test 120 -r /bin/ls
run_basic_test 120 -u /bin/ls
run_basic_test 120 -d /bin/ls
run_basic_test 120 -D /bin/ls

cleanup

report_success
