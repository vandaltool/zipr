#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for grep, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/objdump
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=objdump

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

run_basic_test 120 --help
run_basic_test 120 --version
run_basic_test 120 --doesnotexist
run_basic_test 120 -i
run_basic_test 120 -d /bin/ls
run_basic_test 120 -D /bin/ls
run_basic_test 120 -dz /bin/ls
run_basic_test 120 -d -l /bin/ls
run_basic_test 120 -r /bin/ls
run_basic_test 120 -R /bin/ls
run_basic_test 120 -s /bin/ls
run_basic_test 120 -t /bin/ls
run_basic_test 120 -T /bin/ls
run_basic_test 120 -x /bin/ls
run_basic_test 120 -T -w /bin/ls
run_basic_test 120 -T -w --special-syms /bin/ls
run_basic_test 120 -d -g /bin/ls
run_basic_test 120 -d -f /bin/ls
run_basic_test 120 -d --insn-width=0 /bin/ls
run_basic_test 120 -d --insn-width=8 /bin/ls
run_basic_test 120 -d --insn-width=9 /bin/ls

cleanup

report_success
