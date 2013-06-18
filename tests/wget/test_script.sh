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

#used to detele lins matching one of the following strings (delimited by |
DELETE_FILTER="%|[0-9]{4}-[0-9]{2}-[0-9]{2}|=[0-9]|hello.1|hello.2"

#must import the library here, as it depends on some of the above variables
. $TEST_LIB

cleanup

run_basic_test 30  --help
run_basic_test 30 --version
run_basic_test 30

#basic functionality test
run_test_prog_only 30 127.0.0.1:$PORT_NUM/hello_world.txt
mv $CURRENT_DIR/hello_world.txt $THROW_AWAY_DIR/hello.1
run_bench_prog_only 30 127.0.0.1:$PORT_NUM/hello_world.txt 
mv $CURRENT_DIR/hello_world.txt $THROW_AWAY_DIR/hello.2
compare_std_results
compare_files_no_filtering $THROW_AWAY_DIR/hello.1 $THROW_AWAY_DIR/hello.2
cleanup

#-O IP test
run_test_prog_only 30 -O $THROW_AWAY_DIR/hello.1 127.0.0.1:$PORT_NUM/hello_world.txt
run_bench_prog_only 30 -O $THROW_AWAY_DIR/hello.2 127.0.0.1:$PORT_NUM/hello_world.txt 
compare_std_results
compare_files_no_filtering $THROW_AWAY_DIR/hello.1 $THROW_AWAY_DIR/hello.2
cleanup

#-O HTTP test
run_test_prog_only 30 -O $THROW_AWAY_DIR/hello.1 http://localhost:$PORT_NUM/hello_world.txt
run_bench_prog_only 30 -O $THROW_AWAY_DIR/hello.2 http://localhost:$PORT_NUM/hello_world.txt 
compare_std_results
compare_files_no_filtering $THROW_AWAY_DIR/hello.1 $THROW_AWAY_DIR/hello.2
cleanup

#-O --no-content-disposition test
run_test_prog_only 30 --no-content-disposition -O $THROW_AWAY_DIR/hello.1 http://localhost:$PORT_NUM/hello_world.txt
run_bench_prog_only 30 --no-content-disposition -O $THROW_AWAY_DIR/hello.2 http://localhost:$PORT_NUM/hello_world.txt 
compare_std_results
compare_files_no_filtering $THROW_AWAY_DIR/hello.1 $THROW_AWAY_DIR/hello.2
cleanup

#bad server test
run_basic_test 15 666.666.666.666/hello_world.txt

#bad command test
run_basic_test 15 -lakdjfalkj4 127.0.0.1:1235/hello_world.txt

#no file test
run_basic_test 15 127.0.0.1:1235/does_not_exist
#quite no file test
run_basic_test 15 --quiet 127.0.0.1:1235/does_not_exist

#no file -O test
run_test_prog_only 20 -O $THROW_AWAY_DIR/dummy.1 127.0.0.1:$PORT_NUM/does_not_exist
run_bench_prog_only 20 -O $THROW_AWAY_DIR/dummy.2 127.0.0.1:$PORT_NUM/does_not_exist
compare_std_results
touch $THROW_AWAY_DIR/dummy.2
touch $THROW_AWAY_DIR/dummy.1
compare_files_no_filtering $THROW_AWAY_DIR/dummy.1 $THROW_AWAY_DIR/dummy.2
cleanup

#noop test
run_test_prog_only 30 127.0.0.1:$PORT_NUM/
mv index.html $THROW_AWAY_DIR/index.html.1
run_bench_prog_only 30 127.0.0.1:$PORT_NUM/
mv index.html $THROW_AWAY_DIR/index.html.2
compare_std_results
compare_files_no_filtering $THROW_AWAY_DIR/index.html.1 $THROW_AWAY_DIR/index.html.2
cleanup


#Multi test, -O, no-cache, no-cookies, ignore-length, -E,
run_test_prog_only 45 -O $THROW_AWAY_DIR/hello.1 -E --no-cache --no-cookies --ignore-length http://localhost:$PORT_NUM/
run_bench_prog_only 45 -O $THROW_AWAY_DIR/hello.2 -E --no-cache --no-cookies --ignore-length http://localhost:$PORT_NUM/ 
compare_std_results
compare_files_no_filtering $THROW_AWAY_DIR/hello.1 $THROW_AWAY_DIR/hello.2
cleanup

#default-name test
run_test_prog_only 45 --default-page=hello_world.txt http://localhost:$PORT_NUM/
mv hello_world.txt $THROW_AWAY_DIR/index.html.1
run_bench_prog_only 45 --default-page=hello_world.txt http://localhost:$PORT_NUM/ 
mv hello_world.txt $THROW_AWAY_DIR/index.html.2
compare_std_results
compare_files_no_filtering $THROW_AWAY_DIR/index.html.1 $THROW_AWAY_DIR/index.html.2
cleanup

#prefix test, -P (prefix, i.e., where the file will be stored) and no clobber test
touch $THROW_AWAY_DIR/hello_world.txt
run_test_prog_only 45 -P $THROW_AWAY_DIR -nc  http://localhost:$PORT_NUM/hello_world.txt
touch $THROW_AWAY_DIR/hello_world.txt
run_bench_prog_only 45 -P $THROW_AWAY_DIR -nc  http://localhost:$PORT_NUM/hello_world.txt
compare_std_results


report_success
