#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

TESTPROG=$1

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
run_basic_test 120 -b fox $DATA_DIR/data1.txt
run_basic_test 120 --mmap -i fox $DATA_DIR/data1.txt
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
run_basic_test 120 -w "^[a-zA-Z,0-9].*\[\].*" $DATA_DIR/data1.txt 
run_basic_test 120 "\\body" $DATA_DIR/data1.txt 
run_basic_test 120 -E "(T|t)he*(q|x)uick*" $DATA_DIR/data1.txt 
run_basic_test 120 -U -w -i -c --context=3 "lazy dog" $DATA_DIR/data1.txt 
run_basic_test 120 -f $DATA_DIR/pattern $DATA_DIR/data1.txt 
run_basic_test 120 -f $DATA_DIR/pattern $DATA_DIR/data2.txt 
run_basic_test 120 -i -n -f $DATA_DIR/pattern2 $DATA_DIR/data1.txt 
run_basic_test 120 -i -n -f $DATA_DIR/pattern2 $DATA_DIR/data2.txt 
run_basic_test 120 -s --color -ivn "^the.*" $DATA_DIR/data1.txt 
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
run_basic_test 120 -v "[[:digit:]]\{2\}[ -]\?[[:digit:]]\{10\}" $DATA_DIR/data1.txt 
run_basic_test 120 "[[:digit:]]\{2\}[ -]\?[[:digit:]]\{10\}" $DATA_DIR/data1.txt 
run_basic_test 120 --include="dat*" "^\.[0-9]" -R $DATA_DIR
run_basic_test 120 --include="dat*" --exclude="data2.txt" "^\.[0-9]" -R $DATA_DIR
run_basic_test 120 -z the $DATA_DIR/data1.txt

cat $DATA_DIR/data1.txt | $TESTPROG -i FOX
if [ ! $? -eq 0 ]; then
	report_failure
fi
cat $DATA_DIR/data1.txt | $TESTPROG "[[:digit:]]\{2\}[ -]\?[[:digit:]]\{10\}" 
if [ ! $? -eq 0 ]; then
	report_failure
fi
cat $DATA_DIR/data1.txt | $TESTPROG -i "^the"
if [ ! $? -eq 0 ]; then
	report_failure
fi

run_basic_test 120 -m10 -E -f $DATA_DIR/khadafy.regexp $DATA_DIR/khadafy.lines

#Ben, for some reason this one doesn't work even when I run the original grep against itself
#run_basic_test 120 -E "[a-z]*" $DATA_DIR/data1.txt 

#
# From regression tests shipped with grep
#
# checking for a palindrome
echo "radar" | $TESTPROG -e '\(.\)\(.\).\2\1' > /dev/null 2>&1
if test $? -ne 0 ; then
        echo "Backref: palindrome, test #1 failed"
        report_failure
fi

# hit hard with the `Bond' tests
# For now, remove the `?' in the last parentheses, so that new glibc can do it.  --Stepan
echo "civic" | $TESTPROG -E -e '^(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.).?\9\8\7\6\5\4\3\2\1$' > /dev/null 2>&1
if test $? -ne 0 ; then
        echo "Options: Bond, test #2 failed"
        report_failure
fi

# backref are local should be error
echo "123" | $TESTPROG -e 'a\(.\)' -e 'b\1' > /dev/null 2>&1
if test $? -ne 2 ; then
        echo "Backref: Backref not local, test #3 failed"
        report_failure
fi

# Pattern should fail
echo "123" | $TESTPROG -e '[' -e ']' > /dev/null 2>&1
if test $? -ne 2 ; then
        echo "Backref: Compiled not local, test #4 failed"
        report_failure
fi

# checking for -E extended regex
echo "abababccccccd" | $TESTPROG -E -e 'c{3}' > /dev/null 2>&1
if test $? -ne 0 ; then
        echo "Options: Wrong status code, test \#1 failed"
        report_failure
fi

# checking for basic regex
echo "abababccccccd" | $TESTPROG -G -e 'c\{3\}' > /dev/null 2>&1
if test $? -ne 0 ; then
        echo "Options: Wrong status code, test \#2 failed"
        report_failure
fi

# checking for fixed string
echo "abababccccccd" | $TESTPROG -F -e 'c\{3\}' > /dev/null 2>&1
if test $? -ne 1 ; then
        echo "Options: Wrong status code, test \#3 failed"
        report_failure
fi

echo | $TESTPROG -P '\s*$'

# should return 1 found no match
echo "abcd" | $TESTPROG -E -e 'zbc' > /dev/null 2>&1
if test $? -ne 1 ; then
	echo "Status: Wrong status code, test \#2 failed"
	report_failure
fi
# the filename MMMMMMMM.MMM should not exist hopefully
if test -r MMMMMMMM.MMM; then
        echo "Please remove MMMMMMMM.MMM to run check"
else
        # should return 2 file not found
        grep -E -e 'abc' MMMMMMMM.MMM > /dev/null 2>&1
        if test $? -ne 2 ; then
                echo "Status: Wrong status code, test \#3 failed"
                fail=1
	report_failure
        fi

        # should return 2 file not found
        grep -E -s -e 'abc' MMMMMMMM.MMM > /dev/null 2>&1
        if test $? -ne 2 ; then
                echo "Status: Wrong status code, test \#4 failed"
                fail=1
	report_failure
        fi

        # should return 2 file not found
        echo "abcd" | grep -E -s 'abc' - MMMMMMMM.MMM > /dev/null 2>&1
        if test $? -ne 2 ; then
                echo "Status: Wrong status code, test \#5 failed"
                fail=1
	report_failure
        fi
        # should return 0 found a match
        echo "abcd" | grep -E -q -s 'abc' MMMMMMMM.MMM - > /dev/null 2>&1
        if test $? -ne 0 ; then
                echo "Status: Wrong status code, test \#6 failed"
                fail=1
	report_failure
        fi

        # should still return 0 found a match
        echo "abcd" | grep -E -q 'abc' MMMMMMMM.MMM - > /dev/null 2>&1
        if test $? -ne 0 ; then
                echo "Status: Wrong status code, test \#7 failed"
                fail=1
	report_failure
        fi
fi


cleanup

report_success
