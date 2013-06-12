#!/bin/bash
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#used for filtering program names from output.
ORIG_NAME=zsh

#must import the library here, as it depends on some of the above variables
. $TEST_LIB


# print help and version 
run_basic_test 20 --help
run_basic_test 20 --version

# touch a tmp file
rm -f tmp
run_basic_test 20 -c "/bin/touch tmp"
rm -f tmp
run_basic_test 20 -c "touch tmp"
rm -f tmp

run_basic_test 10 test1.sh
run_basic_test 10 test2.sh
run_basic_test 10 test3.sh
run_basic_test 10 test4.sh

report_success
