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
	local rida_flags="-c rida=on -s meds_static=off "
	local ss_flags="-c stack_stamp=on"
	local p1_flags="-c p1transform=on"

	for src in $src_files
	do
		for option in -O0 -O1 -O2 -O3 -Os -Og
		do
			# stars/ida
			doit_meta $src "$option" ""
			doit_meta $src "$option" "$p1_flags"
			doit_meta $src "$option" "$ss_flags"

			#rida
			doit_meta $src "$option" "$rida_flags "
			doit_meta $src "$option" "$rida_flags $p1_flags"
			doit_meta $src "$option" "$rida_flags $ss_flags"
		done
	done
}

main