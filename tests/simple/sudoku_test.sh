#!/bin/sh

BENCH_NAME=sudoku.exe
BENCH=$SECURITY_TRANSFORMS_HOME/tests/simple/$BENCH_NAME

cp $SECURITY_TRANSFORMS_HOME/tests/simple/sudoku.dat sudoku.dat

rm -f expout experr out err

$BENCH >expout 2>experr
status_exp=$?

$1 >out 2>err
status_actual=$?
echo "out:"
cat out
echo "err:"
cat err
if [ $status_actual -ne $status_exp ]; then
    echo "exit discrepency"
    echo "expected $status_exp, found $status_actual"
    exit 1
fi

diff expout out
if [ $? -ne 0 ]; then
    echo "out discrepency"
    exit 1
fi

diff experr err
if [ $? -ne 0 ]; then
    echo "error discrepency"
    exit 1
fi

exit 0
