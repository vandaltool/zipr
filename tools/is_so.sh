#!/bin/sh

file $1 | grep "LSB shared object" > /dev/null

if [ $? = 0 ]; then
	echo 1
else
	echo 0
fi
