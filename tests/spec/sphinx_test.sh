#!/bin/sh

RUN_DIR=$SECURITY_TRANSFORMS_HOME/tests/spec/sphinx_run/
BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/sphinx_exp/sphinx.out


cd $RUN_DIR

rm -f considered.out total_considered.out act.out act.err

timeout 120 $1 ctlfile . args.an4 >act.out 2>act.err
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

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/sphinx_exp/considered.out

diff $BENCH_FILE considered.out
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: considered.out Output Differs from Expected"
    cd -
    exit 1
fi

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/sphinx_exp/total_considered.out

diff $BENCH_FILE total_considered.out
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: total_considered.out Output Differs from Expected"
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

