#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for w3c, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/w3c
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=w3c

CLEANUP_FILES="w3c.out w3c.1.out w3c.2.out"

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

# comparing stdout directly is tricky a w3c can print out %completion for the output directly on stdout or stderr (this can easily confuse a straight comparison of stdout/stderr between the test and benchmark programs)
# to avoid this problem we specify -n (non-interactive mode)
run_basic_test 120 -n -help 
run_basic_test 120 -single -n -version
run_basic_test 120 -n -z 

run_basic_test 120 -single -n http://127.0.0.1:1235/index.html
run_basic_test 120 -single -n http://127.0.0.1:7333

run_basic_test 120 -head -single -n http://127.0.0.1:7333

run_test_prog_only 120 -o w3c.1.out -single -n http://127.0.0.1:1235/index.html
run_bench_prog_only 120 -o w3c.2.out -single -n http://127.0.0.1:1235/index.html
diff w3c.1.out w3c.2.out
if [ ! $? -eq 0 ]; then
	report_failure
fi

# -source
run_test_prog_only 120 -source -single -n http://127.0.0.1:1235/index.html 
grep -i "peasoup" test_out
if [ ! $? -eq 0 ]; then
	report_failure
fi

# exercise all the verbose flags
run_test_prog_only 120 -n -v abcgpstu http://127.0.0.1:1235 
grep -i "HTanchor" test_error
if [ ! $? -eq 0 ]; then
	report_failure
fi

# -options
run_basic_test 120 -n -single -options http://127.0.0.1:1235 
if [ ! $? -eq 0 ]; then
	report_failure
fi

# just run this one to exercise code path
run_test_prog_only 30 -n -single -put $DATA_DIR/data.txt -dest http://127.0.0.1:1235/foobar/foobar2

#cat $DATA_DIR/data1.txt | run_test_prog_only 120  -i FOX
#cat $DATA_DIR/data1.txt | run_bench_prog_only 120  -i FOX
#compare_std_results

run_basic_test 120 -to www/source -single -n http://127.0.0.1:1235/index.html
run_basic_test 120 -to www/mime -single -n http://127.0.0.1:1235/index.html 
run_basic_test 120 -cl http://127.0.0.1:1235/index.html  | grep -i content

cleanup

report_success
