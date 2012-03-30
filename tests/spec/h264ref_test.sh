#!/bin/sh

TEST_DIR=$SECURITY_TRANSFORMS_HOME/tests/spec/h264ref_exp
BENCH_FILE=$TEST_DIR/h264_exp.out
CONFIG=$TEST_DIR/foreman_test_encoder_baseline.cfg
TIMEOUT=300

cd $TEST_DIR
rm -f act.out act.err
echo "timeout $TIMEOUT $1 -d $CONFIG >h264ref_act.out 2>> h264ref_act.err"
timeout $TIMEOUT $1 -d $CONFIG >act.out 2>>act.err
status=$?
if [ $status -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Exit Status Non-Zero: $status"
    cd -
    exit 1
fi

diff $BENCH_FILE act.out
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Output Differs from Expected"
    cd -
    exit 1
fi

rm -f empty
touch empty

diff act.err empty
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Error Differs from Expected"
    cd -
    exit 1
fi

echo "TEST WRAPPER SUCCESS"
cd -
exit 0
