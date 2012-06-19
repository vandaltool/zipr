#!/bin/sh

INPUT_DIR=$SPEC_HOME/benchspec/CPU2006/403.gcc/data/ref/input
BENCH_DIR=$SECURITY_TRANSFORMS_HOME/tests/spec/gcc_exp

cleanup()
{
    rm -rf act.out act.err empty
}

validate()
{
    status=$1
    orig_out=$2
    rm -f empty
    touch empty

    if [ $status -ne 0 ]; then
	echo "TEST WRAPPER FAILURE: Exit Status Non-Zero: $status"
	cleanup
	exit 1
    fi

    diff empty act.err

    if [ $? -ne 0 ];then
	echo "TEST WRAPPER FAILURE: Error Output Differs from Expected"
	cleanup
	exit 1
    fi

    diff act.out $orig_out
    if [ $? -ne 0 ];then
	echo "TEST WRAPPER FAILURE: Actual Output Differs from Expected"
	cleanup
	exit 1
    fi
    
}

cleanup
echo "$1 $INPUT_DIR/166.i -o act.out 2>act.err"
$1 $INPUT_DIR/166.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/166.s

cleanup
echo "$1 $INPUT_DIR/scilab.i -o act.out 2>act.err"
$1 $INPUT_DIR/scilab.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/scilab.s

cleanup
echo "$1 $INPUT_DIR/200.i -o act.out 2>act.err"
$1 $INPUT_DIR/200.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/200.s

cleanup
echo "$1 $INPUT_DIR/c-typeck.i -o act.out 2>act.err"
$1 $INPUT_DIR/c-typeck.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/c-typeck.s

cleanup
echo "$1 $INPUT_DIR/cp-decl.i -o act.out 2>act.err"
$1 $INPUT_DIR/cp-decl.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/cp-decl.s

cleanup
echo "$1 $INPUT_DIR/expr.i -o act.out 2>act.err"
$1 $INPUT_DIR/expr.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/expr.s

cleanup
echo "$1 $INPUT_DIR/expr2.i -o act.out 2>act.err"
$1 $INPUT_DIR/expr2.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/expr2.s

cleanup
echo "$1 $INPUT_DIR/g23.i -o act.out 2>act.err"
$1 $INPUT_DIR/g23.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/g23.s

cleanup
echo "$1 $INPUT_DIR/s04.i -o act.out 2>act.err"
$1 $INPUT_DIR/s04.i -o act.out 2>act.err
status=$?
validate $status $BENCH_DIR/s04.s

cleanup

echo "TEST WRAPPER SUCCESS"
exit 0




