#!/bin/sh

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/milc_exp/milc.out
INPUT_FILE=$SPEC_HOME/benchspec/CPU2006/433.milc/data/train/input/su3imp.in



rm -f act.out act.err
echo "timeout 120 $1 $INPUT_FILE >act.out 2>act.err"
timeout 120 $1 <$INPUT_FILE >act.out 2>act.err
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