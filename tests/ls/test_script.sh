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

run_basic_test 120 --help
run_basic_test 120 --version
run_basic_test 120 --doesnotexist
run_basic_test 120 ls $DATA_DIR
run_basic_test 120 ls $DATA_DIR/not-exist
run_basic_test 120 ls -a $DATA_DIR
run_basic_test 120 ls -l $DATA_DIR
run_basic_test 120 ls -ltr $DATA_DIR
run_basic_test 120 ls -ltrR $DATA_DIR
run_basic_test 120 ls -ha $DATA_DIR
run_basic_test 120 ls -Z $DATA_DIR
run_basic_test 120 ls -Rd $DATA_DIR
run_basic_test 120 ls -Rlg $DATA_DIR
run_basic_test 120 ls -lC -s $DATA_DIR
run_basic_test 120 ls -C --color=always $DATA_DIR
run_basic_test 120 ls -F $DATA_DIR
run_basic_test 120 ls -Fg $DATA_DIR
run_basic_test 120 ls -ha --si $DATA_DIR
run_basic_test 120 ls -ha --si -i $DATA_DIR
run_basic_test 120 ls -lt --time-style=full-iso $DATA_DIR
run_basic_test 120 ls -lt --time-style=long-iso $DATA_DIR
run_basic_test 120 ls -lt --time-style=iso $DATA_DIR
run_basic_test 120 ls -lt --tabsize=4 --time-style=iso $DATA_DIR
run_basic_test 120 ls -ltZ $DATA_DIR

cleanup

report_success
