#!/bin/bash   
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#used for filtering program names from output.
ORIG_NAME=zsh

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

export TEST_DIR=$PEASOUP_HOME/tests/zsh

# print help and version 
#run_basic_test 20 --help
#run_basic_test 20 --version

# touch a tmp file
#rm -f tmp
run_basic_test 20 -c '/usr/bin/touch tmp'
rm -f tmp
run_basic_test 20 -c 'touch tmp'
rm -f tmp

run_basic_test 20 -c 'echo hello'
run_basic_test 20 -c 'hello() { echo hello }; hello'
run_basic_test 20 -c 'hello() { echo $* }; hello bob'

run_basic_test 10 $TEST_DIR/tests/test1.sh
run_basic_test 10 $TEST_DIR/tests/test2.sh
run_basic_test 10 $TEST_DIR/tests/test3.sh
run_basic_test 10 $TEST_DIR/tests/test4.sh
run_basic_test 10 $TEST_DIR/tests/test5.sh
run_basic_test 10 $TEST_DIR/tests/test5.sh bobby

report_success
