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
CLEANUP_FILES="$THROW_AWAY_DIR/* wget-log* index.html*"
ORIG_NAME="wget"
LOG_DIR=$TEST_DIR/log

rm -rf $LOG_DIR
mkdir $LOG_DIR

CURRENT_DIR=`pwd`

PORT_NUM=1235
DELETE_FILTER="%|2013-|hello.1|hello.2"

#must import the library here, as it depends on some of the above variables
. $TEST_LIB



cleanup

run_basic_test 30  --help
run_basic_test 30 --version
run_basic_test 30

#basic functionality test
run_test_prog_only 60 127.0.0.1:$PORT_NUM/hello_world.txt
mv $CURRENT_DIR/hello_world.txt $DATA_DIR/throw_away/hello.1
run_bench_prog_only 60 127.0.0.1:$PORT_NUM/hello_world.txt 
mv $CURRENT_DIR/hello_world.txt $DATA_DIR/throw_away/hello.2
compare_std_results
compare_files_no_filtering $DATA_DIR/throw_away/hello.1 $DATA_DIR/throw_away/hello.2
cleanup

#-O test
run_test_prog_only 60 -O $DATA_DIR/throw_away/hello.1 127.0.0.1:$PORT_NUM/hello_world.txt
run_bench_prog_only 60 -O $DATA_DIR/throw_away/hello.2 127.0.0.1:$PORT_NUM/hello_world.txt 
compare_std_results
compare_files_no_filtering $DATA_DIR/throw_away/hello.1 $DATA_DIR/throw_away/hello.2
cleanup

#bad server test
run_basic_test 15 666.666.666.666/hello_world.txt

#bad command test
run_basic_test 15 -lakdjfalkj4 127.0.0.1:1235/hello_world.txt

#no file test
run_basic_test 15 127.0.0.1:1235/does_not_exist

#noop test
run_test_prog_only 60 127.0.0.1:$PORT_NUM/
mv index.html $THROW_AWAY_DIR/index.html.1
run_bench_prog_only 60 127.0.0.1:$PORT_NUM/
mv index.html $THROW_AWAY_DIR/index.html.2
#compare_exit_status
compare_std_results
compare_files_no_filtering $THROW_AWAY_DIR/index.html.1 $THROW_AWAY_DIR/index.html.2



report_success
