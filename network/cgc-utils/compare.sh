#!/bin/bash

i=0
i_max=269
j=0
j_max=269
while [ $i -lt $i_max ]; do
	for j in $(seq $i $j_max); do
		diff -q $i $j > /dev/null
		if [ $? -ne 0 ]; then
			echo "$i,$j"
			i=$((j-1))
			break
		fi	
	done
	i=$((i+1))
done
