#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for grep, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/sort
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=sort

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

# sanity check
SORTED=/tmp/tmp.sorted.$(whoami)
timeout 20 $BENCH $DATA_DIR/data.cba > $SORTED
diff $SORTED $DATA_DIR/data.abc
if [ ! $? -eq 0 ]; then
	report_failure
fi
rm $SORTED

run_basic_test 120 --help
run_basic_test 120 --version
run_basic_test 120 --doesnotexist
run_basic_test 120 -d $DATA_DIR/data.txt
run_basic_test 120 -d -b $DATA_DIR/data.txt
run_basic_test 120 -f $DATA_DIR/data.txt
run_basic_test 120 -g $DATA_DIR/data.txt
run_basic_test 120 -g -i $DATA_DIR/data.txt
run_basic_test 120 -M $DATA_DIR/data.txt
run_basic_test 120 -h $DATA_DIR/data.txt
run_basic_test 120 -n $DATA_DIR/data.txt

# non-deterministic tests
run_bench_prog_only 120 -R $DATA_DIR/data.txt
run_test_prog_only 120 -R $DATA_DIR/data.txt
compare_exit_status

run_bench_prog_only 120 -R --random-source=$DATA_dir/random.txt $DATA_DIR/data.txt
run_test_prog_only 120 -R --random-source=$DATA_dir/random.txt $DATA_DIR/data.txt
compare_exit_status

run_basic_test 120 $DATA_DIR/data.txt
run_basic_test 120 --reverse $DATA_DIR/data.txt
run_basic_test 120 -V $DATA_DIR/data.txt
run_basic_test 120 -c -u $DATA_DIR/data.txt
run_basic_test 120 -z -c -u $DATA_DIR/data.txt
run_basic_test 120 -V --debug $DATA_DIR/data.txt
run_basic_test 120 -V -S 512 --debug $DATA_DIR/data.txt

cleanup

report_success
