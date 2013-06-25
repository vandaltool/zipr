#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#for bzip, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/nginx
DATA_DIR=$TEST_DIR/data 
THROW_AWAY_DIR=$TEST_DIR/throw_away
CLEANUP_FILES="$THROW_AWAY_DIR/* $TEST_DIR/data/logs/* index.html*"
ORIG_NAME="nginx"
#LOG_DIR=$TEST_DIR/log

#rm -rf $LOG_DIR
#mkdir $LOG_DIR

CURRENT_DIR=`pwd`

PORT_NUM=1025

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

cleanup

echo "BEGIN NGINX TESTING"

# help
echo "Basic test -h"
run_basic_test 30 -h
echo "Finished Basic test -h"

# version
echo "Basic test -v"
run_basic_test 30 -v
echo "Finished Basic test -h"

# version and configure options
echo "Basic test -V"
run_basic_test 30 -V
echo "Finished Basic test -V"

# test configuration and exit
echo "Basic test -t"
run_basic_test 30 -t
echo "Finished Basic test -h"

# basic functionality tests
#############
#  test1.sh #
#############
# start nginx unmodified original
# set the prefix to point at $TEST_DIR/
echo Running original server program 
run_server_bench_prog_only 60 nginx $TEST_DIR/tests/test1.sh -p $TEST_DIR/data/ -c $TEST_DIR/data/conf/nginx.conf 

# Wait a little bit
sleep 5 

# Run the modified program
echo Running modified server program
run_server_test_prog_only 60 nginx $TEST_DIR/tests/test1.sh -p $TEST_DIR/data/ -c $TEST_DIR/data/conf/nginx.conf 

# compare the results
compare_files_no_filtering $THROW_AWAY_DIR/good-01.orig/index.html $THROW_AWAY_DIR/good-01.test/index.html
echo $? is the return status

#############
#  test2.sh #
#############
echo Running original server program 
run_server_bench_prog_only 60 nginx $TEST_DIR/tests/test2.sh -p $TEST_DIR/data/ -c $TEST_DIR/data/conf/nginx.conf 

# Wait a little bit
sleep 5 

# Run the modified program
echo Running modified server program
run_server_test_prog_only 60 nginx $TEST_DIR/tests/test2.sh -p $TEST_DIR/data/ -c $TEST_DIR/data/conf/nginx.conf 

# compare the results
compare_files_no_filtering $THROW_AWAY_DIR/good-02.orig/pageone.html $THROW_AWAY_DIR/good-02.test/pageone.html

echo test.sh return status: $? 

#############
#  test3.sh #
#############
echo Running original server program 
run_server_bench_prog_only 60 nginx $TEST_DIR/tests/test3.sh -p $TEST_DIR/data/ -c $TEST_DIR/data/conf/nginx.conf 

# Wait a little bit
sleep 5 

# Run the modified program
echo Running modified server program
run_server_test_prog_only 60 nginx $TEST_DIR/tests/test3.sh -p $TEST_DIR/data/ -c $TEST_DIR/data/conf/nginx.conf 

# compare the results
compare_files_no_filtering $THROW_AWAY_DIR/good-03.orig/pagetwo.html $THROW_AWAY_DIR/good-03.test/pagetwo.html
echo test3.sh return status: $? 
#############
#  test4.sh #
#############
echo Running original server program 
run_server_bench_prog_only 60 nginx $TEST_DIR/tests/test4.sh -p $TEST_DIR/data/ -c $TEST_DIR/data/conf/nginx.conf 

# Wait a little bit
sleep 5 

# Run the modified program
echo Running modified server program
run_server_test_prog_only 60 nginx $TEST_DIR/tests/test4.sh -p $TEST_DIR/data/ -c $TEST_DIR/data/conf/nginx.conf 

# compare the results
compare_files_no_filtering $THROW_AWAY_DIR/good-04.orig/pagethree.html $THROW_AWAY_DIR/good-04.test/pagethree.html
echo test4.sh return status: $? 

report_success "END OF NGINX TESTING"
