#!/bin/sh

# test dir
TEST_DIR=$PEASOUP_HOME/tests/nginx

# scripts dir
SCRIPT_DIR=$TEST_DIR/scripts

# first arg must be the type of test (orig or test using the manual_test_lib lingo)
TEST_TYPE=$1
echo TEST_TYPE is $TEST_TYPE

if [[ "$TEST_TYPE" == "" ]]; then
    echo "ERROR:  test type (orig or test) must be specified:  You specified: $TEST_TYPE"
    exit 1
fi

PORT_NUM=1025

# run the test
# first make sure that there are no intermediate files leftover
rm -rf $TEST_DIR/throw_away/good-01.$TEST_TYPE

$SCRIPT_DIR/service_mon.sh localhost $PORT_NUM
wget -P $TEST_DIR/throw_away/good-01.$TEST_TYPE http://localhost:$PORT_NUM/index.html

# exit with the exit status of the wget request
exit $?
