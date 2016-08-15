#!/bin/bash

for i in `ls | grep queue`
do
	inputs_count=$(ls $i | wc -l);
	cat $i/* > /dev/null 2>&1;
	if [ $? -eq 1 ];
	then
		echo "$i,0,0,0";
		continue;
	fi;
	input_bytes=$(cat $i/* | wc -c);
	average=$(echo "scale=2; ${input_bytes}/${inputs_count}" | bc);
	echo "${i},${inputs_count},${input_bytes},${average}";
done
