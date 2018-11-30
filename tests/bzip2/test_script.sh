#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#for bzip, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/bzip2
DATA_DIR=$TEST_DIR/data

#for bzip, I create some additional files I will need to have cleaned up when it is appropriate to do so. Cleanup is handled automatically by the test lib when success or failure is reported, when run_basic_test is called or if cleanup is called manually. 
CLEANUP_FILES="$DATA_DIR/compression_input1_test.bz2 $DATA_DIR/compression_input1_orig.bz2 $DATA_DIR/compression_input1_orig $DATA_DIR/compression_input1_test"

#used for filtering program names from output.
#since we might use this script for all bzip variants, list them all
#but separate by an escaped | for the regex used by manual_test_lib
ORIG_NAME="bzip\|bunzip\|bzip2\|bunzip2"

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

#Test 1, I give a bogus input of ^ to bzip
run_basic_test 20  ^
#Test 2, help test
run_basic_test 20 --help
#Test 3, version test
run_basic_test 20 --version
#Test 4, basic decompression test (to stdout)
run_basic_test 120 -dc $DATA_DIR/decompression_input1
#Test 5, basic compression test (to stdout)
run_basic_test 120 -zc $DATA_DIR/compression_input1
#Test 6, decompression with -s option
run_basic_test 120 -dcs $DATA_DIR/decompression_input1
#Test 7, compression with -s option
run_basic_test 120 -zcs $DATA_DIR/compression_input1
#Test 8, testing -5 option with compression
run_basic_test 120 -zc -5 $DATA_DIR/compression_input1
#Test 9, testing -t option
run_basic_test 120 -t  $DATA_DIR/decompression_input1

#The following test does not use run_basic_test, instead, I want to
#test the ability of bzip to write a file, not stdout. To do this, I call run_test_prog_only and run_bench_prog_only. These two functions take the same inputs as run_basic_test, but no comparisons are done for either call. These functions just run the test and bench programs respectively. I then call compare_std_results, which looks for the stdout, stderr, and exit status for both bench and test runs, and does a comparison much like run_basic_test. Following this, I then do a comparison manually of the expected output files. Note that I check if the -i flag had been specified before doing the comparison. Also note that I call cleanup following the test. 

#run_bench_prog_only will not do anything if the -i flag is set, neither will compare_std_results, so no check is needed in this program for -i. 

#set up the input to compress for the modified program
cp $DATA_DIR/compression_input1 $DATA_DIR/compression_input1_test
#-k keeps the original input, 
run_test_prog_only 120 -k $DATA_DIR/compression_input1_test
#set up the input to compress for comparison
cp $DATA_DIR/compression_input1 $DATA_DIR/compression_input1_orig
run_bench_prog_only 120 -k $DATA_DIR/compression_input1_orig
#It is possible we are not running bzip, i.e., we didn't determine the program
#correctly, in which case the expected output file wont exist, so first make
#sure it does exist before doing any comparisons. I check the bench's output
#file because it is possible for the test to fail to produce an output for
#different reasons. 
if [[ -z "$IGNORE_RESULTS" ]] ; then
	if [[ -f $DATA_DIR/compression_input1_orig.bz2 ]]; then
		compare_std_results
		
		diff $DATA_DIR/compression_input1_test.bz2 $DATA_DIR/compression_input1_orig.bz2
		status=$?
		if [ ! "$status" -eq 0 ]; then
			echo "Exit Status Failure"
			report_failure
		fi 
	else
		report_failure
	fi
fi
cleanup
report_success
