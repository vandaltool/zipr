#!/bin/bash
BENCH_NAME=rm_O3.exe
ORIG_NAME=rm
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

rm -f tmp

touch empty.txt
touch tmp
$1 tmp 2>error.txt
if [ ! $? -eq 0 ]; then
    echo "Basic Sanity Test Exit Status Failure"
    report_failure
fi 

diff empty.txt error.txt

if [ ! $? -eq 0 ]; then
    echo "Basic Sanity Test Error Status Failure"
    report_failure
fi 

rm -rf  error.txt tmp

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

test_transform rm/cycle
test_transform rm/dangling-symlink
test_transform rm/deep-1
test_transform rm/deep-2
test_transform rm/dir-no-w
test_transform rm/dir-nonrecur
test_transform rm/dot-rel
test_transform rm/isatty
test_transform rm/empty-inacc
test_transform rm/empty-name
test_transform rm/f-1
test_transform rm/fail-eacces
test_transform rm/fail-eperm
test_transform rm/i-1
test_transform rm/i-never
test_transform rm/i-no-r
test_transform rm/ignorable
test_transform rm/inaccessible
test_transform rm/interactive-always
test_transform rm/interactive-once
test_transform rm/ir-1
test_transform rm/r-1
test_transform rm/r-2
test_transform rm/r-3
test_transform rm/r-4
test_transform rm/readdir-bug
test_transform rm/rm1
test_transform rm/rm2
test_transform rm/rm3
test_transform rm/rm4
test_transform rm/rm5
test_transform rm/sunos-1
test_transform rm/unread2
test_transform rm/unread3
test_transform rm/unreadable
test_transform rm/v-slash

restore

report_success