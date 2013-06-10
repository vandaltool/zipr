#!/bin/bash
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh
TEST_DIR=$PEASOUP_HOME/tests/bzip
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=bzip

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

run_basic_test 20  ^
run_basic_test 20 --help
run_basic_test 20 --version

run_basic_test 120 -dc $DATA_DIR/decompression_input1

run_basic_test 120 -zc $DATA_DIR/compression_input1

run_basic_test 120 -dcs $DATA_DIR/decompression_input1

run_basic_test 120 -zcs $DATA_DIR/compression_input1

run_basic_test 120 -zc -5 $DATA_DIR/compression_input1

run_basic_test 120 -t  $DATA_DIR/decompression_input1

cp $DATA_DIR/compression_input1 $DATA_DIR/compression_input1_test
run_test_prog_only 120 -k $DATA_DIR/compression_input1_test
cp $DATA_DIR/compression_input1 $DATA_DIR/compression_input1_orig
run_bench_prog_only 120 -k $DATA_DIR/compression_input1_orig
compare_std_results
rm -f $DATA_DIR/compression_input1_orig $DATA_DIR/compression_input1_test
if [[ -z "$IGNORE_RESULTS" ]]; then
	echo "120 -k $DATA_DIR/compression_input1_test"
	
	diff $DATA_DIR/compression_input1_test.bz2 $DATA_DIR/compression_input1_orig.bz2
	status=$?

	rm -f $DATA_DIR/compression_input1_*
	if [ ! "$status" -eq 0 ]; then
		echo "Exit Status Failure"
		report_failure
	fi 
fi
report_success