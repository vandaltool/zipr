#!/bin/bash -x

binary=$1
function=$2
filename=$3
malloc=$4

tmp=$filename.tmp.$$

tr -s '\r\n' ' ' < $filename | sed -e 's/ $/\n/' > $tmp
addresses=`cat $tmp`
rm $tmp

echo "addresses: $addresses"

for addr in $addresses
do
	echo "=== Investigating maybe $function at address $addr"
	if [ -z $malloc ]; then	
		cmd="timeout 2 $PEASOUP_HOME/cinderella/cinderella.exe $binary $function $addr"  
	else
		cmd="timeout 2 $PEASOUP_HOME/cinderella/cinderella.exe $binary $function $addr --malloc $malloc"  
		if [ "$addr" = "$malloc" ]; then
			echo "prince negative $function $addr"
			continue
		fi
	fi
	echo $cmd
	$cmd
	if [ $? -eq 0 ]; then
		echo "prince positive $function $addr"
	else
		if [ $? -eq 1 ]; then
			echo "prince negative $function $addr"
		else
			echo "prince invalid $function $addr"
		fi
	fi
	killall $binary
done 

killall $binary
echo "Done processing function $function with filename $filename"

#cat $filename | awk '{print $1}' | while read name;
