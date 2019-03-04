#!/bin/bash

#
# Extract eip from valid CGC core file
# Outputs eip value in hex, e.g., 0x804823c
#

bin=$1
core=$2

if [ ! -f $bin ]; then
	echo "binary file: $bin does not exist"
	exit 1
fi

if [ ! -f $core ]; then
	echo "core file: $core does not exist"
	exit 1
fi

if [ -z $PS_READELF ]; then
	echo "Environment variable PS_READELF is not defined"
	exit 1
fi

$PS_READELF -h $core | grep -i Type | grep -i core &> /dev/null
if [ ! $? -eq 0 ]; then
	echo "$core is not a valid core file"
	exit 1
fi

gdb $1 $2 --batch --ex "x/i \$eip" 2>&1 | grep -i "cannot access"
if [ $? -eq 0 ]; then
	echo "crashed but eip is bogus"
	exit 1
fi

gdb $1 $2 --batch --ex "info registers eip" | grep eip | awk -F " " '{print $2;}'
