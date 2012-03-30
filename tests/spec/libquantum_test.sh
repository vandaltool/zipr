#!/bin/sh

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/libquantum_exp/libquantum.out

rm -f act.out act.err
timeout 120 $1 143 25 >act.out 2>act.err

status=$?
if [ $status -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Exit Status Non-Zero: $status"
    exit 1
fi

diff $BENCH_FILE act.out
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Output Differs from Expected"
    exit 1
fi

rm -f empty
touch empty

diff act.err empty
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Error Differs from Expected"
    exit 1
fi

echo "TEST WRAPPER SUCCESS"
exit 0


