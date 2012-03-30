#!/bin/sh

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/mcf_exp/mcf.out
INPUT_FILE=$SPEC_HOME/benchspec/CPU2006/429.mcf/data/train/input/inp.in


rm -f act.out act.err
echo "timeout 120 $1 $INPUT_FILE >act.out 2>act.err"
timeout 120 $1 $INPUT_FILE >act.out 2>act.err
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