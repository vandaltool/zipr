#!/bin/bash

src_files=" simple_throw.cpp throw.cpp derived_throw.cpp derived2_throw.cpp  derived3_throw.cpp derived4_throw.cpp "
throws="THROW_INT THROW_CHAR THROW_FLOAT"

#---------------

compare()
{

	rm -f good.txt xform.txt 

	env $1 ./a.out > good.txt 2>&1 
	exit1=$?
	env $1 ./xxx > xform.txt 2>&1 
	exit2=$?

	cmp good.txt xform.txt > /dev/null 2>&1

	if [[ $? != 0 ]] || [[ $exit1 != $exit2 ]]; then
		echo "Failed test: $1"; 
		diff good.txt xform.txt
		exit 2
	fi
}

doit()
{
	src=$1
	options="$2"
	psopts="$3"

	echo  "------------------------------------------------------"
	echo "Trying $src with options: $options"
	echo "And psflags=$psopts "
	g++ -w $options $src 
	rm -Rf peasoup_executable_direc*
	(set -x ; EHIR_VERBOSE=1 $PSZ ./a.out ./xxx --step-option fill_in_indtargs:--split-eh-frame --step-option zipr:'--add-sections true' 	$psopts)

	compare

	for throw in $throws
	do
		compare $throw=1	
	done
	echo "Passed test!"
	echo  "------------------------------------------------------"
}


doit_meta()
{
	src=$1
	option="$2"
	psopts="$3"

	doit $src "$option  " "$psopts"
	doit $src "$option -fPIC " "$psopts"
	doit $src "$option -fPIC -fomit-frame-pointer" "$psopts"
	doit $src "$option -fPIC  -pie" "$psopts"
	doit $src "$option -fPIC -fomit-frame-pointer -pie" "$psopts"
}

main()
{

	for src in $src_files
	do
		for option in -O0 -O1 -O2 -O3 -Os -Og
		do
			doit_meta $src "$option" ""
			#doit_meta $src "$option" "--step p1transform=on"
			#doit_meta $src "$option" "--step stack_stamp=on"
		done
	done
}

main
