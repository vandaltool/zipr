#!/bin/sh

#
# don't pass me flags or i'll smack you.
#
for i in $*
do
	echo $i|egrep "^-";
	if [ 0 -eq  $? ] ; then 
		echo SMACK\! No flags to this script, only files to link
		exit 1
	fi
	if [ ! -f $i ]; then
		echo File $i not found
		exit 2	
	fi
done

gcc -Bstatic -static $* 

