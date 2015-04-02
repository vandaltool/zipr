#!/bin/bash -x
#
# Find <libc_function> in <binary> using functions in <filename> as candidates
#

variant_id=$1
binary=$2                # binary augmented via test loop
libc_function=$3         # what libc function are we looking for?
filename=$4              # file containing names of candidate functions in binary
malloc=$5                # (optional) assume $4 is malloc

tmp=$filename.tmp.$$
binarycopy=$$.$(basename $binary)
cp $binary $binarycopy
binary=$binarycopy

echo "binary copy is: $binary"

prince_driver=$SECURITY_TRANSFORMS_HOME/tools/prince/prince_driver.exe

tr -s '\r\n' ' ' < $filename | sed -e 's/ $/\n/' > $tmp
functions_to_test=`cat $tmp`
rm $tmp

echo "functions_to_test: $functions_to_test"

for function_to_test in $functions_to_test
do
	echo "=== Investigating maybe $libc_function with function $function_to_test"
	if [ -z $malloc ]; then	
		cmd="timeout 2 $prince_driver $variant_id $binary $libc_function $function_to_test"  
	else
		cmd="timeout 2 $prince_driver $variant_id $binary $libc_function $function_to_test --malloc $malloc"  
		if [ "$function_to_test" = "$malloc" ]; then
			echo "prince negative $libc_function $function_to_test"
			continue
		fi
	fi
	echo $cmd
	$cmd
	if [ $? -eq 0 ]; then
		echo "prince positive $libc_function $function_to_test"
	else
		if [ $? -eq 1 ]; then
			echo "prince negative $libc_function $function_to_test"
		else
			echo "prince invalid $libc_function $function_to_test"
		fi
	fi
	killall `basename $binary`
done 

killall `basename $binary`
echo "Done processing libc_function $libc_function with filename $filename"
