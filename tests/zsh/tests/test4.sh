#!/bin/zsh

echo td=$TEST_DIR

while read j
do
    echo $j
done < ${TEST_DIR}/tests/test2.sh

while read j
do
    echo $j
done < ${TEST_DIR}/tests/../tests/test2.sh


