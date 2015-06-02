#!/bin/bash -x

#
# Run program under gdb
# Detect seg faults
# Make sure valid address
# Extract eip
#

bin=$1
input_file=$2

timeout=60

if [ ! -f $bin ]; then
	echo "binary file: $bin does not exist"
	exit 1
fi

if [ ! -f $bin ]; then
	echo "input file: $input_file does not exist"
	exit 1
fi

# look for segmentation fault
timeout $timeout gdb $1 --batch --ex "run < $2" --ex "info registers \$eip" --ex "quit" > gdb.out 2>&1 < /dev/null
grep -i segmentation gdb.out &>/dev/null 
if [ $? -eq 0 ]; then
    eip=`grep eip gdb.out | awk -F " " '{print $2;}'`
    # does eip point to a valid instruction?
    gdb $1 --batch --ex "x/i $eip" --ex "quit" 2>&1 </dev/null | grep -i "cannot access" &>/dev/null
    if [ $? -eq 0 ]; then
       echo "crashed but eip $eip does not point to a valid instruction" 
       exit 1
    fi
    echo $eip
    exit 0
else
    cat gdb.out
fi

exit 1
