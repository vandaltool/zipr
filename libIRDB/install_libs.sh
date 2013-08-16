#!/bin/sh 

cd lib; 
for i in *; 
do 
	if [ ! -f ../../lib/$i -o $i -nt ../../lib/$i ]; then 
		echo Installing $i
		cp $i ../../lib/; 
	fi
done

