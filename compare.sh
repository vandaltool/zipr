#!/bin/bash

ORIGINAL_CB=$1
REPLACEMENT_CB=$2

INPUT=test

BASE_OUTPUT=$(echo $INPUT | timeout 5s $1)
BASE_RESULT=$?
RCB_OUTPUT=$(echo $INPUT | timeout 5s $2)
RCB_RESULT=$?

echo $BASE_RESULT
echo $RCB_RESULT

if [ $BASE_RESULT -ne $RCB_RESULT ]; then
	exit 1
fi
if [ "$BASE_OUTPUT" != "$RCB_OUTPUT" ]; then
	exit 1
fi
exit 0
