#!/bin/bash

src_files="derived4_throw.cpp derived3_throw.cpp derived2_throw.cpp  derived_throw.cpp simple_throw.cpp throw.cpp "
throws="THROW_INT THROW_CHAR THROW_FLOAT"

#---------------

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

doit()
{
	src=$1
	shift
	options=$*

	echo "Trying $src with options: $options "
	g++ -w $options $src 
	rm -Rf peasoup_executable_direc*
	EHIR_VERBOSE=1 $PSZ ./a.out ./xxx --step-option fill_in_indtargs:--split-eh-frame --step-option zipr:'--add-sections true' 	

	compare

	for throw in $throws
	do
		compare $throw=1	
	done
}


doit_meta()
{
	src=$1
	shift
	option=$*

			doit $src $option 
			doit $src $option -fPIC 
			doit $src $option -fPIC -fomit-frame-pointer
			doit $src $option -fPIC  -pie
			doit $src $option -fPIC -fomit-frame-pointer -pie
}

main()
{

	for src in $src_files
	do
		doit_meta $src "-O0"
		doit_meta $src "-O1"
		doit_meta $src "-O2"
		doit_meta $src "-O3"
		doit_meta $src "-Os"
	done
}

main
