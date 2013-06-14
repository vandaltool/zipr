#!/bin/bash
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#used for filtering program names from output.
ORIG_NAME=zsh

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

TEST_DIR=$PEASOUP_HOME/tests/tcpdump



# print help and version 
run_basic_test 20 --help
run_basic_test 20 --version

run_basic_test 20  -t -n -v -v -v -r $TEST_DIR/lmp.pcap 

for i in x xx X XX A AA; do
        run_basic_test 20 -$i -s0 -nr $TEST_DIR/tcpd_tests/print-flags.pcap 
done



# now run typical tests
cat $TEST_DIR/tcpd_tests/TESTLIST | while read name input output options
do
  case $name in
      \#*) continue;;
      '') continue;;
  esac

  run_basic_test 20 "-n -r $TEST_DIR/tcpd_tests/$input $options"

done


report_success
