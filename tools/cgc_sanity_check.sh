#!/bin/bash

ORIGINAL_CB=$1
REPLACEMENT_CB=$2

INPUT=test

BASE_OUTPUT=$(echo $INPUT | timeout 5s $1)
BASE_RESULT=$?
RCB_OUTPUT=$(echo $INPUT | timeout 5s $2)
RCB_RESULT=$?
BASE_OUTPUT2=$(echo $INPUT | timeout 5s $1)
BASE_RESULT2=$?

if [ $BASE_RESULT -gt 128 ] && [ $BASE_RESULT -lt 200 ]; then
	BASE_CRASHED=1
else
	BASE_CRASHED=0
fi

if [ $BASE_RESULT2 -gt 128 ] && [ $BASE_RESULT2 -lt 200 ]; then
	BASE_CRASHED2=1
else
	BASE_CRASHED2=0
fi

if [ $RCB_RESULT -gt 128 ] && [ $RCB_RESULT -lt 200 ]; then
	RCB_CRASHED=1
else
	RCB_CRASHED=0
fi

echo "First base cb run:" $BASE_RESULT
echo "Second base cb run:" $BASE_RESULT2
echo "Replacement cb run:" $RCB_RESULT
echo $BASE_CRASHED
echo $BASE_CRASHED2
echo $RCB_CRASHED

if [ $BASE_RESULT -eq $BASE_RESULT2 ]; then
	if [ $BASE_RESULT -ne $RCB_RESULT ]; then
		exit 1
	fi
fi
if [ "$BASE_OUTPUT" == "$BASE_OUTPUT2" ]; then
	if [ "$BASE_OUTPUT" != "$RCB_OUTPUT" ]; then
		exit 1
	fi
fi
if [ $RCB_CRASHED -ne 0 ] && [ $BASE_CRASHED -eq 0 ] && [ $BASE_CRASHED2 -eq 0 ]; then
	exit 1
fi
exit 0
