#!/bin/bash  

cd lib; 
for i in *; 
do 
	if test ! -f ../../lib/$i -o $i -nt ../../lib/$i ; then 
		echo Installing $i
		cp $i ../../lib/; 
	fi
done

