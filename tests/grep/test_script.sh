#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh


#for grep, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/grep
DATA_DIR=$TEST_DIR/data

#used for filtering program names from output.
ORIG_NAME=grep

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

run_basic_test 120 --help
run_basic_test 120 --version
run_basic_test 120 --doesnotexist
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
run_basic_test 120 -U -w -i -c --context=3 "lazy dog" $DATA_DIR/data1.txt 
run_basic_test 120 -f $DATA_DIR/pattern $DATA_DIR/data1.txt $DATA_DIR/data2.txt
run_basic_test 120 -i -n -f $DATA_DIR/pattern2 $DATA_DIR/data1.txt $DATA_DIR/data2.txt
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

cat $DATA_DIR/data1.txt | run_test_prog_only 120  -i FOX
cat $DATA_DIR/data1.txt | run_bench_prog_only 120  -i FOX
compare_std_results

#cat $DATA_DIR/data1.txt | run_test_prog_only 120  "[[:digit:]]\{2\}[ -]\?[[:digit:]]\{10\}" 
#cat $DATA_DIR/data1.txt | run_bench_prog_only 120  "[[:digit:]]\{2\}[ -]\?[[:digit:]]\{10\}" 
#compare_std_results

cat $DATA_DIR/data1.txt | run_test_prog_only 120 -i "^the"
cat $DATA_DIR/data1.txt | run_bench_prog_only 120 -i "^the"
compare_std_results

run_basic_test 120 -m10 -E -f $DATA_DIR/khadafy.regexp $DATA_DIR/khadafy.lines

printf 'foo\nbar\n' | run_test_prog_only 120 -z -q 'foo[[:space:]]\+bar'
printf 'foo\nbar\n' | run_bench_prog_only 120 -z -q 'foo[[:space:]]\+bar'
compare_std_results

#
# From regression tests shipped with grep
#
# checking for a palindrome
echo "radar" | run_test_prog_only 120 -e '\(.\)\(.\).\2\1' 
echo "radar" | run_bench_prog_only 120 -e '\(.\)\(.\).\2\1'
compare_std_results

# backref are local should be error
echo "123" | run_test_prog_only 120 -e 'a\(.\)' -e 'b\1'
echo "123" | run_bench_prog_only 120 -e 'a\(.\)' -e 'b\1'
compare_std_results

# Pattern should fail
echo "123" | run_test_prog_only 120 -e '[' -e ']'
echo "123" | run_bench_prog_only 120 -e '[' -e ']' 
compare_std_results

# checking for -E extended regex
echo "abababccccccd" | run_test_prog_only 120 -E -e 'c{3}'
echo "abababccccccd" | run_bench_prog_only 120 -E -e 'c{3}'
compare_std_results

# checking for basic regex
echo "abababccccccd" | run_test_prog_only 120 -G -e 'c\{3\}'
echo "abababccccccd" | run_bench_prog_only 120 -G -e 'c\{3\}'
compare_std_results

# checking for fixed string
echo "abababccccccd" | run_test_prog_only 120 -F -e 'c\{3\}'
echo "abababccccccd" | run_bench_prog_only 120 -F -e 'c\{3\}'
compare_std_results

echo | run_test_prog_only 120 -P '\s*$'
echo | run_bench_prog_only 120 -P '\s*$'
compare_std_results

# should return 1 found no match
echo "abcd" | run_test_prog_only 120 -E -e 'zbc'
echo "abcd" | run_bench_prog_only 120 -E -e 'zbc'
compare_std_results

# should return 0 found a match
echo "abcd" | run_test_prog_only 120 -E -q -s 'abc' MMMMMMMM.MMM 
echo "abcd" | run_bench_prog_only 120 -E -q -s 'abc' MMMMMMMM.MMM 
compare_std_results

run_basic_test 120 -E '(T|t)he.*(q|x)uick.*' $DATA_DIR/data1.txt 

# hit hard with the `Bond' tests
# For now, remove the `?' in the last parentheses, so that new glibc can do it.  --Stepan
echo "civic" | run_test_prog_only 120 -E -e '^(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.).?\9\8\7\6\5\4\3\2\1$'
echo "civic" | run_bench_prog_only 120 -E -e '^(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.?)(.).?\9\8\7\6\5\4\3\2\1$'
compare_std_results

#
# BUG -- these don't work
#

#run_basic_test 120 -E '[a-z]*' $DATA_DIR/data1.txt 

cleanup

report_success
