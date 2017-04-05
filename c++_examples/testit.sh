#!/bin/bash


compare()
{
	env $1 ./a.out > good.txt 2>&1 
	env $1 ./xxx > xform.txt 2>&1 

	cmp good.txt xform.txt > /dev/null 2>&1

	if [ $? != 0 ]; then
		echo "Failed test: $1"; 
		diff good.txt xform.txt
		exit 2
	fi
}

src_files="simple_throw.cpp throw.cpp"
options="-Os -O0 -O1 -O2 -O3"
throws="THROW_INT THROW_CHAR THROW_FLOAT"

for src in $src_files
do
	echo "Trying $src"

	for option in $options
	do
		echo "With $option"
		g++ $option $src
		rm -Rf peasoup_executable_direc*
		EHIR_VERBOSE=1 $PSZ ./a.out ./xxx --step-option fill_in_indtargs:--split-eh-frame --step-option zipr:'--add-sections true' 	

		compare

		for throw in $throws
		do
			compare $throw=1	
		done
		
	done
done
