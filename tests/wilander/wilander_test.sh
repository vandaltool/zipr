#!/bin/sh

timeout 5 $1 -4
status=$?
touch empty

if [ $status -eq 139 ];then
    echo TEST WRAPPER FAILURE
    exit 1
fi

timeout 5 $1 -3
status=$?
touch empty

if [ $status -eq 139 ];then
    echo TEST WRAPPER FAILURE
    exit 1
fi

timeout 5 $1 1
status=$?
touch empty

if [ $status -eq 139 ];then
    echo TEST WRAPPER FAILURE
    exit 1
fi

timeout 5 $1 2
status=$?
touch empty

if [ $status -eq 139 ];then
    echo TEST WRAPPER FAILURE
    exit 1
fi

timeout 5 $1 3
status=$?
touch empty

if [ $status -eq 139 ];then
    echo TEST WRAPPER FAILURE
    exit 1
fi

timeout 5 $1 4
status=$?
touch empty

if [ $status -eq 139 ];then
    echo TEST WRAPPER FAILURE
    exit 1
fi

echo TEST WRAPPER SUCCESS

exit 0