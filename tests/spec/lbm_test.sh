#!/bin/sh

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/lbm_exp/lbm.out
INPUT_FILE=$SPEC_HOME/benchspec/CPU2006/470.lbm/data/ref/input/100_100_130_ldc.of

rm -f act.out act.err
echo "$1 100 reference.dat 0 0 $INPUT_FILE >act.out 2>act.errr"

$1 100 reference.dat 0 0 $INPUT_FILE >act.out 2>act.err
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


