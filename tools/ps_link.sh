#!/bin/sh

#
# don't pass me flags or i'll smack you.
# exceptions: -o, -l, -L, is OK
#
for i in $*
do
	echo $i|egrep "^-";	# check for starting with a - 
	if [ 0 -eq  $? ] ; then 
		echo $i|egrep "^-o" > /dev/null;	# check for starting with a -o 
		dasho=$?
		echo $i|egrep "^-l" > /dev/null;	# check for starting with a -l
		dashl=$?
		echo $i|egrep "^-L" > /dev/null;	# check for starting with a -L 
		dashL=$?

		if [ 0 -eq  $dasho ] ; then 
			echo -n; # blank on purpose
		elif [ 0 -eq  $dashl ] ; then 
			echo -n; # blank on purpose
		elif [ 0 -eq  $dashL ] ; then 
			echo -n; # blank on purpose
		else
			echo SMACK\! No flags to this script, only files to link
			exit 1
		fi
	fi
done

gcc -Bstatic -static $* 
retval=$?
exit $retval
