#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for grep, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/ls
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=ls

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"


# sanity check
$BENCH / | grep tmp >/dev/null 2>&1
if [ ! $? -eq 0 ]; then
	report_failure 
fi

run_basic_test 20 --help
run_basic_test 20 --version
run_basic_test 20 --doesnotexist
run_basic_test 20 $DATA_DIR
run_basic_test 20 $DATA_DIR/not-exist
run_basic_test 20 -a $DATA_DIR
run_basic_test 20 -l $DATA_DIR
run_basic_test 20 -ltr $DATA_DIR
run_basic_test 20 -ltrR $DATA_DIR
run_basic_test 20 -ha $DATA_DIR
run_basic_test 20 -Z $DATA_DIR
run_basic_test 20 -Rd $DATA_DIR
run_basic_test 20 -Rlg $DATA_DIR
run_basic_test 20 -lC -s $DATA_DIR
run_basic_test 20 -C --color=always $DATA_DIR
run_basic_test 20 -F $DATA_DIR
run_basic_test 20 -Fg $DATA_DIR
run_basic_test 20 -ha --si $DATA_DIR
run_basic_test 20 -ha --si -i $DATA_DIR
run_basic_test 20 -lt --time-style=full-iso $DATA_DIR
run_basic_test 20 -lt --time-style=long-iso $DATA_DIR
run_basic_test 20 -lt --time-style=iso $DATA_DIR
run_basic_test 20 -lt --tabsize=4 --time-style=iso $DATA_DIR
run_basic_test 20 -ltZ $DATA_DIR

cleanup

report_success
