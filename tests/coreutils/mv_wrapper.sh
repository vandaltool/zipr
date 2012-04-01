#!/bin/bash
BENCH_NAME=mv_O3.exe
ORIG_NAME=mv
BENCH=$SECURITY_TRANSFORMS_HOME/tests/coreutils/$BENCH_NAME
TOP_LEVEL=$SECURITY_TRANSFORMS_HOME/tests/coreutils/coreutils-7.4
TRANSFORM_DEST=$TOP_LEVEL/src/$ORIG_NAME
TEST_DIR=$TOP_LEVEL/tests
ORIG_BACKUP_NAME=$TOP_LEVEL/src/$ORIG_NAME.bak

BENCH_REGEX=`echo $BENCH | sed 's/\./\\\./'`
BENCH_NAME_REGEX=`echo $BENCH_NAME | sed 's/\./\\\./'`

save_original()
{
    echo "STASHING AWAY ORIGINAL"
    cp $TRANSFORM_DEST $ORIG_BACKUP_NAME
}

restore()
{
    if [ -f $ORIG_BACKUP_NAME ]; then
	cp $ORIG_BACKUP_NAME $TRANSFORM_DEST
    fi
}

install_transformed()
{
    echo "INSTALL TRANSFORMED BINARY"
    cp $1 $TRANSFORM_DEST
}

report_failure()
{
    echo "TEST WRAPPER FAILED"
    exit 1
}

report_success()
{
    echo "TEST WRAPPER SUCCESS"
    exit 0
}

test_transform()
{
    tmp_dir=`pwd`
    cd $TEST_DIR
    rm -f check.out
    make check TESTS=$1 >check.out
    cat check.out
    grep FAIL check.out | grep -v XFAIL

    status=$?

    if [ $status -eq 0 ]; then
	rm check.out
	restore
	cd $tmp_dir
	report_failure
    fi
}


rm -rf empty.txt error.txt tmp tmp2

touch empty.txt
touch tmp
$1 tmp tmp2 2>error.txt
if [ ! $? -eq 0 ]; then
    echo "Basic Sanity Test Exit Status Failure"
    report_failure
fi 

diff empty.txt error.txt

if [ ! $? -eq 0 ]; then
    echo "Basic Sanity Test Error Status Failure"
    report_failure
fi 

rm -rf  error.txt tmp tmp2

$BENCH --help >out1.txt
$1 --help >out2.txt 2>error.txt

if [ ! $? -eq 0 ]; then
    echo "Help Test Exit Status Failure"
    report_failure
fi 

cat out2.txt | sed "s^/.*a\.stratafied^$BENCH_REGEX^" | sed "s^a\.stratafied^$BENCH_NAME_REGEX^g" >tmp
cat tmp >out2.txt

diff empty.txt error.txt
if [ ! $? -eq 0 ]; then
    echo "Help Test Error Stderr  Failure"
    report_failure
fi 

diff out1.txt out2.txt
if [ ! $? -eq 0 ]; then
    echo "Help Test Error Stdout Failure"
    report_failure
fi 

rm  error.txt tmp

$BENCH --version >out1.txt
$1 --version >out2.txt 2>error.txt
if [ ! $? -eq 0 ]; then
    echo "Version Test Exit Status Failure"
    report_failure
fi 


cat out2.txt | sed "s^/.*a\.stratafied^$BENCH_REGEX^" | sed "s^a\.stratafied^$BENCH_NAME_REGEX^g" >tmp
cat tmp >out2.txt

diff empty.txt error.txt
if [ ! $? -eq 0 ]; then
    echo "Version Test Error Stderr  Failure"
    report_failure
fi 

diff out1.txt out2.txt
if [ ! $? -eq 0 ]; then
    echo "Version Test Error Stdout Failure"
    report_failure
fi 


$BENCH -^ >out1.txt 2>error1.txt
status=$?
$1 -^ >out2.txt 2>error2.txt
if [ ! $? -eq $status ]; then
    echo "Invalid Arg Test Exit Status Failure"
    report_failure
fi 


cat out2.txt | sed "s^/.*a\.stratafied^$BENCH_REGEX^" | sed "s^a\.stratafied^$BENCH_NAME_REGEX^g" >tmp
cat tmp >out2.txt

cat error2.txt | sed "s^/.*a\.stratafied^$BENCH_REGEX^" | sed "s^a\.stratafied^$BENCH_NAME_REGEX^g" >tmp
cat tmp >error2.txt


diff error1.txt error2.txt
if [ ! $? -eq 0 ]; then
    echo "Invalid Arg Test Error Stderr  Failure"
    report_failure
fi 

diff out1.txt out2.txt
if [ ! $? -eq 0 ]; then
    echo "Invalid Arg Test Error Stdout Failure"
    report_failure
fi


$BENCH  >out1.txt 2>error1.txt
status=$?
$1  >out2.txt 2>error2.txt
if [ ! $? -eq $status ]; then
    echo "Usage Test Exit Status Failure"
    report_failure
fi 


cat out2.txt | sed "s^/.*a\.stratafied^$BENCH_REGEX^" | sed "s^a\.stratafied^$BENCH_NAME_REGEX^g" >tmp
cat tmp >out2.txt

cat error2.txt | sed "s^/.*a\.stratafied^$BENCH_REGEX^" | sed "s^a\.stratafied^$BENCH_NAME_REGEX^g" >tmp
cat tmp >error2.txt


diff error1.txt error2.txt
if [ ! $? -eq 0 ]; then
    echo "Usage Test Error Stderr  Failure"
    report_failure
fi 

diff out1.txt out2.txt
if [ ! $? -eq 0 ]; then
    echo "Usage Test Error Stdout Failure"
    report_failure
fi

rm -rf  error.txt tmp out1.txt out2.txt empty.txt error1.txt error2.txt


save_original
install_transformed $1

test_transform mv/atomic
test_transform mv/atomic2
test_transform mv/backup-dir
test_transform mv/backup-is-src
test_transform mv/childproof
test_transform mv/diag
test_transform mv/dir-file
test_transform mv/dir2dir
test_transform mv/dup-source
test_transform mv/force
test_transform mv/hard-2
test_transform mv/hard-3
test_transform mv/hard-4
test_transform mv/hard-link-1
test_transform mv/hard-verbose
test_transform mv/i-1
test_transform mv/i-2
test_transform mv/i-3
test_transform mv/i-4
test_transform mv/i-5
test_transform mv/i-link-no
test_transform mv/into-self
test_transform mv/into-self-2
test_transform mv/into-self-3
test_transform mv/into-self-4
test_transform mv/mv-n
test_transform mv/mv-special-1
test_transform mv/no-target-dir
test_transform mv/part-fail
test_transform mv/part-hardlink
test_transform mv/part-rename
test_transform mv/part-symlink
test_transform mv/partition-perm
test_transform mv/perm-1
test_transform mv/to-symlink
test_transform mv/trailing-slash
test_transform mv/update

restore
report_success