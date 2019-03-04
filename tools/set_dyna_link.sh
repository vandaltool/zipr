#!/bin/sh

infile=$1
outfile=$2


rm -f $outfile

error(){
	echo "This program ($infile) is not a statically or dynamically linked ELF file"
	exit 1 
}

usage() {
	echo "Usage: set_dyna_link.sh exe outfile"
	exit 2
}


ldd $infile  2>&1 |grep "not a dynamic executable" 2>&1 > /dev/null

# if selected lines found 
if [ $? = 0 ]; then
	# We know it's not a DL file  
	# check that it's statically linked ELF

	file $infile  2>&1 |grep "ELF 32-bit LSB executable" |grep "statically linked" 2>&1 > /dev/null

	# selected lines found
	if [ $? = 0 ]; then
		echo $infile is detected as a 32-bit dynamically linked executable.
		echo n > $outfile
		exit 0
	else
		error
	fi

else

	file $infile  2>&1 |grep "ELF 32-bit LSB executable" |grep "dynamically linked" 2>&1 > /dev/null
	# selected lines found
	if [ $? = 0 ]; then
		echo $infile is detected as a 32-bit statically linked executable.
		echo y > $outfile
		exit 0
	else
		error
	fi
fi

echo Shouldnt reach here.
exit 3
