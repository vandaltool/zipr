#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#for grep, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/ncal

#used for filtering program names from output.
ORIG_NAME=ncal

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

#run_basic_test will run both the modified program and original program using the same arguments. The first argument ot run_basic_test is the timeout value (in seconds) followed by the arguments for the programs. run_basic_test also does comparisons of stdout, stderr, and the exit status, and only does comparisons and uses timeout when the -i flag is not set. See the library for more details. 

pwd
echo "TEST_PROG: $TEST_PROG"

ARGS=("" "-h" "-bogus" "2000" "-w 2000" "-3 5 2001" "1 2000" "1 1999" "-j 2000" "-J 2000" "-S 1000" "-M 1015" "-J -o 12 9999" "-e -p -y 2000" "-M -y -B9 -A5 2000" "-S -y -B10 -A10 2000" "-1" "-A-20 -B-5 1000"  "-w -m 3 1111" "-N 2000" "-N -w 2000" "-N -3 8 2001" "-N 1 2000" "-N 1 1999" "-N -j 2000" "-N -J 2000" "-N -S 1000" "-N -M 1015" "-N -J -o 12 9999" "-N -e -p -y 2000" "-N -M -y -B9 -A5 2000" "-N -S -y -B10 -A10 2000" "-N -1" "-N -A-20 -B-5 1000"  "-N -w -m 3 1111" "-N -y 1999 -m 3" "-N -y -b" "-N -s GB 2000")

for ix in ${!ARGS[*]}
do
	run_basic_test 30 ${ARGS[$ix]}
done

cleanup

report_success
