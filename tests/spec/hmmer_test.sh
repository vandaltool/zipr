#!/bin/sh
INPUT_FILE=$SPEC_HOME/benchspec/CPU2006/456.hmmer/data/test/input/bombesin.hmm
BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/hmmer_exp/hmmer.out
 

rm -f act.out act.err
timeout 120 $1 --fixed 0 --mean 325 --num 45000 --sd 200 --seed 0 $INPUT_FILE >act.out 2>act.err
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
