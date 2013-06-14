#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#!/bin/bash
#Everyone must point to the manual test library
TEST_LIB=$PEASOUP_HOME/tests/manual_test_lib.sh

#for bzip, I use some data files, so I set up variables pointing to that location
TEST_DIR=$PEASOUP_HOME/tests/wget
DATA_DIR=$TEST_DIR/data 
THROW_AWAY_DIR=$DATA_DIR/throw_away
CLEANUP_FILES=$THROW_AWAY_DIR/*
ORIG_NAME="wget"

CURRENT_DIR=`pwd`

PORT_NUM=1235

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

cleanup
run_test_prog_only 60 127.0.0.1:$PORT_NUM/hello_world.txt
mv $CURRENT_DIR/hello_world.txt $DATA_DIR/throw_away/hello.1
run_bench_prog_only 60 127.0.0.1:$PORT_NUM/hello_world.txt 
mv $CURRENT_DIR/hello_world.txt $DATA_DIR/throw_away/hello.2
compare_exit_status
compare_files_no_filtering $DATA_DIR/throw_away/hello.1 $DATA_DIR/throw_away/hello.2
cleanup

run_test_prog_only 60 -O $DATA_DIR/throw_away/hello.1 127.0.0.1:$PORT_NUM/hello_world.txt
run_bench_prog_only 60 -O $DATA_DIR/throw_away/hello.2 127.0.0.1:$PORT_NUM/hello_world.txt 
compare_exit_status
compare_files_no_filtering $DATA_DIR/throw_away/hello.1 $DATA_DIR/throw_away/hello.2
cleanup

report_success