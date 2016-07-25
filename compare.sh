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

echo "First base cb run:" $BASE_RESULT
echo "Second base cb run:" $BASE_RESULT2
echo "Replacement cb run:" $RCB_RESULT

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
exit 0
