#!/bin/sh

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/bzip_exp/input_program.out
COMPRESSION_FILE=$SPEC_HOME/benchspec/CPU2006/401.bzip2/data/all/input/input.program

rm -f bzip_act bzip_err
echo "timeout $TIMEOUT $1 $COMPRESSION_FILE 10 >bzip_act 2>bzip_err"
timeout 120 $1 $COMPRESSION_FILE 10 >bzip_act 2>bzip_err
status=$?
if [ $status -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Exit Status Non-Zero: $status"
    exit 1
fi

diff $BENCH_FILE bzip_act
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Output Differs from Expected"
    exit 1
fi

rm -f empty
touch empty

diff bzip_err empty
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Error Differs from Expected"
    exit 1
fi

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/bzip_exp/byoudoin.out
COMPRESSION_FILE=$SPEC_HOME/benchspec/CPU2006/401.bzip2/data/train/input/byoudoin.jpg

rm -f bzip_act bzip_err
echo "timeout $TIMEOUT $1 $COMPRESSION_FILE 5 >bzip_act 2>bzip_err"
timeout 120 $1 $COMPRESSION_FILE 5 >bzip_act 2>bzip_err
status=$?
if [ $status -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Exit Status Non-Zero: $status"
    exit 1
fi

diff $BENCH_FILE bzip_act
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Output Differs from Expected"
    exit 1
fi

rm -f empty
touch empty

diff bzip_err empty
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Error Differs from Expected"
    exit 1
fi

BENCH_FILE=$SECURITY_TRANSFORMS_HOME/tests/spec/bzip_exp/input_combined.out
COMPRESSION_FILE=$SPEC_HOME/benchspec/CPU2006/401.bzip2/data/all/input/input.combined

rm -f bzip_act bzip_err
echo "timeout $TIMEOUT $1 $COMPRESSION_FILE 80 >bzip_act 2>bzip_err"
timeout 240 $1 $COMPRESSION_FILE 80 >bzip_act 2>bzip_err
status=$?
if [ $status -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Exit Status Non-Zero: $status"
    exit 1
fi

diff $BENCH_FILE bzip_act
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Output Differs from Expected"
    exit 1
fi

rm -f empty
touch empty

diff bzip_err empty
if [ $? -ne 0 ];then
    echo "TEST WRAPPER FAILURE: Actual Error Differs from Expected"
    exit 1
fi

echo "TEST WRAPPER SUCCESS"
exit 0


