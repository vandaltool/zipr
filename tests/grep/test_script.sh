#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#for bzip, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/grep
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=grep

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

run_basic_test 120 --help
run_basic_test 120 --version
run_basic_test 120 -i -U -B 1 -A 2 brown $DATA_DIR/data1.txt
run_basic_test 120 fox $DATA_DIR/data1.txt
run_basic_test 120 --text -vn Fox $DATA_DIR/data1.txt $DATA_DIR/data2.txt
run_basic_test 120 -iw the $DATA_DIR/data1.txt $DATA_DIR/data2.txt
run_basic_test 120 -RTo --line-buffered the $DATA_DIR
run_basic_test 120 -il pursuit $DATA_DIR/data1.txt $DATA_DIR/data2.txt
run_basic_test 120 --line-buffered -iL pursuit $DATA_DIR/data1.txt $DATA_DIR/data2.txt
run_basic_test 120 -hR "over.*" $DATA_DIR
run_basic_test 120 -i "over.*" $DATA_DIR/data1.txt 
run_basic_test 120 -i -q ".*over.*" $DATA_DIR/data1.txt 
run_basic_test 120 -i -s ".*over.*" $DATA_DIR/data1.txt 
run_basic_test 120 --binary -i -w "over" $DATA_DIR/data1.txt 
run_basic_test 120 -i --line-number ".*dog.*$" $DATA_DIR/data1.txt 
run_basic_test 120 -i "^the.*" $DATA_DIR/data1.txt 
run_basic_test 120 -E "^[a-z,0-9,F-Z]+" $DATA_DIR/data1.txt 
run_basic_test 120 -E "(T|t)he*(q|x)uick*" $DATA_DIR/data1.txt 
run_basic_test 120 -U -w -i -c --context=3 "lazy dog" $DATA_DIR/data1.txt 
run_basic_test 120 -f $DATA_DIR/pattern $DATA_DIR/data1.txt 
run_basic_test 120 -i -n -f $DATA_DIR/pattern2 $DATA_DIR/data1.txt 
run_basic_test 120 --color -ivn "^the.*" $DATA_DIR/data1.txt 
run_basic_test 120 "[:alnum:]" $DATA_DIR/data1.txt 
run_basic_test 120 "[:alpha:]" $DATA_DIR/data1.txt 
run_basic_test 120 "[:digit:]" $DATA_DIR/data1.txt 
run_basic_test 120 "[:lower:]" $DATA_DIR/data1.txt 
run_basic_test 120 "[:space:]" $DATA_DIR/data1.txt 
run_basic_test 120 "[:upper:]" $DATA_DIR/data1.txt 
run_basic_test 120 "\<b...n\>" $DATA_DIR/data1.txt 
run_basic_test 120 "^\.[0-9]" $DATA_DIR/data1.txt 
run_basic_test 120 -E '[[:digit:]]{1,3}\.[[:digit:]]{1,3}\.[[:digit:]]{1,3}\.[[:digit:]]{1,3}' $DATA_DIR/data1.txt 
run_basic_test 120 -E '2{2}' $DATA_DIR/data1.txt 
run_basic_test 120 -E 'h{1}' $DATA_DIR/data1.txt 
run_basic_test 120 -E 'co{1,2}l' $DATA_DIR/data1.txt 
run_basic_test 120 -E 'c{3,}' $DATA_DIR/data1.txt 
run_basic_test 120 -v -E "[[:digit:]]\{2\}[ -]\?[[:digit:]]\{10\}" $DATA_DIR/data1.txt 
run_basic_test 120 -E "[[:digit:]]\{2\}[ -]\?[[:digit:]]\{10\}" $DATA_DIR/data1.txt 



#Ben, for some reason this one doesn't work even when I run the original grep against itself
#run_basic_test 120 -E "[a-z]*" $DATA_DIR/data1.txt 

cleanup
report_success
