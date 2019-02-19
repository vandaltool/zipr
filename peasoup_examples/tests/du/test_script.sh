#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for grep, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/du
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=du

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

timeout 10 $BENCH /usr | grep bin
if [ ! $? -eq 0 ]; then
	report_failure
fi

run_basic_test 120 --help
run_basic_test 120 --version
run_basic_test 120 --doesnotexist
run_basic_test 120 $DATA_DIR
run_basic_test 120 -0 $DATA_DIR
run_basic_test 120 --null $DATA_DIR
run_basic_test 120 -a $DATA_DIR
run_basic_test 120 --all $DATA_DIR
run_basic_test 120 --all --apparent-size $DATA_DIR
run_basic_test 120 -a -BM $DATA_DIR
run_basic_test 120 -b $DATA_DIR
run_basic_test 120 -c $DATA_DIR
run_basic_test 120 --summarize $DATA_DIR
run_basic_test 120 -bh $DATA_DIR
run_basic_test 120 --all -h $DATA_DIR
run_basic_test 120 -L $DATA_DIR
run_basic_test 120 -l $DATA_DIR
run_basic_test 120 -m $DATA_DIR
run_basic_test 120 -P $DATA_DIR
run_basic_test 120 -S $DATA_DIR
run_basic_test 120 --si $DATA_DIR
run_basic_test 120 -s $DATA_DIR
run_basic_test 120 -t $DATA_DIR
run_basic_test 120 --time $DATA_DIR
run_basic_test 120 --time-style=full-iso $DATA_DIR
run_basic_test 120 --time-style=long-iso $DATA_DIR
run_basic_test 120 --time-style=iso $DATA_DIR
run_basic_test 120 --exclude-from=$DATA_DIR/yo.txt $DATA_DIR
run_basic_test 120 --exclude=foobar $DATA_DIR
run_basic_test 120 --exclude="*.txt" $DATA_DIR
run_basic_test 120 -x $DATA_DIR



cleanup

report_success
