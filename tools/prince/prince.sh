#!/bin/bash -x

binary=$1                # binary augmented via test loop
libc_function=$2         # what libc function are we looking for?
filename=$3              # file containing names of candidate functions in binary
malloc=$4                # (optional) assume $4 is malloc

tmp=$filename.tmp.$$

tr -s '\r\n' ' ' < $filename | sed -e 's/ $/\n/' > $tmp
functions_to_test=`cat $tmp`
rm $tmp

echo "functions_to_test: $functions_to_test"

for function_to_test in $functions_to_test
do
	echo "=== Investigating maybe $libc_function with function $function_to_test"
	if [ -z $malloc ]; then	
		cmd="timeout 2 $PEASOUP_HOME/prince/prince_driver.exe $binary $libc_function $function_to_test"  
	else
		cmd="timeout 2 $PEASOUP_HOME/prince/prince_driver.exe $binary $libc_function $function_to_test --malloc $malloc"  
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
	killall $binary
done 

killall $binary
echo "Done processing libc_function $libc_function with filename $filename"

#cat $filename | awk '{print $1}' | while read name;
