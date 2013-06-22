#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for w3c, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/w3c
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=w3c

CLEANUP_FILES="w3c.out w3c.1.out w3c.2.out w3c.log"
DELETE_FILTER="Date:|0x|socket|secs|seconds|maxsock|maxfds|hash value is [0-9]+|write: [0-9]+"

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

# comparing stdout directly is tricky a w3c can print out %completion for the output directly on stdout or stderr (this can easily confuse a straight comparison of stdout/stderr between the test and benchmark programs)
# to avoid this problem we specify -n (non-interactive mode)
#run_basic_test 45 
#run_basic_test 45 -help 
#run_basic_test 45 -version
run_basic_test 45 -z 

echo "aaa"
run_basic_test 10
#run_test_prog_only 10
#grep -i "libwww" test_out 
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

#Anh, your help test is problematic, I tried running basic test on it
#but the test will periodically fail. I am removing the test for now. 
#echo "bbb"
#run_basic_test 10 -help
#run_test_prog_only 10 -help
#grep -i "libwww" test_out 
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

echo "ccc"
run_basic_test 10 -version
#run_test_prog_only 10 -version
#grep -i "libwww" test_out
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

run_basic_test 45 -maxforwards 2 -single -n http://127.0.0.1:1235/index.html
run_basic_test 45 -single -n http://127.0.0.1:7333

run_basic_test 45 -head -single -n http://127.0.0.1:7333

run_basic_test 45 -o w3c.1.out -single -n http://127.0.0.1:1235/index.html
run_basic_test 45 -o w3c.2.out -single -n http://127.0.0.1:1235/index.html

#run_test_prog_only 45 -o w3c.1.out -single -n http://127.0.0.1:1235/index.html
#run_bench_prog_only 45 -o w3c.2.out -single -n http://127.0.0.1:1235/index.html
#diff w3c.1.out w3c.2.out
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

run_basic_test 45 -o w3c.out -timeout 30 http://127.0.0.1:1235/index.html 
#grep -i "peasoup" w3c.out
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

run_basic_test  45 -timeout 30 -get http://127.0.0.1:1235/index.html 
#grep -i "peasoup" test_out
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

# -source
run_basic_test 45 -timeout 30 -source -single -n http://127.0.0.1:1235/index.html 
#grep -i "peasoup" test_out
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

# exercise all the verbose flags
#run_test_prog_only 45 -n -vabcgpstu http://127.0.0.1:1235 
#grep -i "HTanchor" test_error
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

run_basic_test 45 -n -vabcgpstu http://127.0.0.1:1235 

run_basic_test 45 -n -vbpstu http://127.0.0.1:1235 
#run_test_prog_only 45 -n -vbpstu http://127.0.0.1:1235 
#grep -i "channel" test_error
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi

# -options
run_basic_test 45 -n -single -options http://127.0.0.1:1235


# just run this one to exercise code path
run_basic_test 45 -n -put $DATA_DIR/data.txt -dest http://127.0.0.1:1235/testing/
run_basic_test 45 -n -auth user:password@realm http://127.0.0.1:1235/peasoup.auth 
run_basic_test 45 -n -delete http://127.0.0.1:1235/peasoup.auth/doesnotexist.html 
run_basic_test 45 -post http://127.0.0.1:1235/testing/index.html -form "RECORD=ID" "COL1=a" "COL2=b" "COL3=c" "COL4=d"
run_basic_test 45 -r $DATA_DIR/bogus.conf http://127.0.0.1:1235/testing/index.html 


#cat $DATA_DIR/data1.txt | run_test_prog_only 45  -i FOX
#cat $DATA_DIR/data1.txt | run_bench_prog_only 45  -i FOX
#compare_std_results

run_basic_test 45 -to www/source -single -n http://127.0.0.1:1235/index.html
run_basic_test 45 -to www/mime -single -n http://127.0.0.1:1235/index.html 
run_basic_test 45 -cl http://127.0.0.1:1235/index.html  | grep -i content

run_basic_test 45  -l w3c.log http://127.0.0.1:1235/index.html 

#run_test_prog_only 45 -l w3c.log http://127.0.0.1:1235/index.html 
#grep -i 1235 w3c.log
#if [ ! $? -eq 0 ]; then
#	report_failure
#fi


cleanup

report_success
