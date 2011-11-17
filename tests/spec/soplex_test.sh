#!/bin/sh

BENCH_NAME=soplex.exe
BENCH=$SECURITY_TRANSFORMS_HOME/tests/spec/$BENCH_NAME

$BENCH $SPEC_HOME/benchspec/CPU2006/450.soplex/data/test/input/test.mps >expout 2>experr
exp_status=$?

$1 $SPEC_HOME/benchspec/CPU2006/450.soplex/data/test/input/test.mps >out 2>err
act_status=$?

grep -v .*time.* out >tmp
cat tmp >out
grep -v .*time.* expout >tmp
cat tmp >expout

grep -v .*Time.* out >tmp
cat tmp >out
grep -v .*Time.* expout >tmp
cat tmp >expout

echo "out:"
cat out
echo "err:"
cat err

if [ $exp_status -ne $act_status ]; then
    echo "expected: $exp_status actual: $act_status"
    echo "Test Wrapper Failure: status failure"
    exit 1
fi


diff expout out

if [ $? -ne 0 ]; then
    echo "Test Wrapper Failure: status failure"
    exit 1
fi

diff experr err

if [ $? -ne 0 ]; then
    echo "Test Wrapper Failure: status failure"
    exit 1
fi

echo "Test Wrapper Success"

exit 0
