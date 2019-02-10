#!/bin/bash

set -e
trap clean EXIT

cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
source set_env_vars
cd ./security_transforms/tools/selective_cfi/tests/cicd_tests/foo_src

source ../../cfi_smokescreen_configs.sh 

get_correct()
{
	cp libfoo.so.orig libfoo.so
	./foo.exe > correct
	echo $? >> correct
}

do_test()
{
	echo Running test: "$1" "$2"
	
	set +e
	cp $2 libfoo.so  
	./$1 > out 
	echo $? >> out

	cmp out correct
        res=$?
        if [[ $res != 0 ]]; then
                echo "detected output diff:"
                diff out correct
                exit 1
        fi
	set -e

}


build()
{
	gcc -o libfoo.so libfoo.c -w -shared -fPIC
	gcc -o foo.exe foo.c -w -L. -lfoo
	mv libfoo.so libfoo.so.orig

	gcc -o libfoo.O.so libfoo.c -O -w -shared -fPIC
        gcc -o libfoo.O2.so libfoo.c -O2 -w -shared -fPIC
        gcc -o libfoo.O3.so libfoo.c -O3 -w -shared -fPIC
        gcc -o libfoo.Os.so libfoo.c -Os -w -shared -fPIC
        mv libfoo.O.so libfoo.O.so.orig
        mv libfoo.O2.so libfoo.O2.so.orig
        mv libfoo.O3.so libfoo.O3.so.orig
        mv libfoo.Os.so libfoo.Os.so.orig
}


protect()
{
	files=(foo.exe libfoo.so.orig libfoo.O.so.orig libfoo.O2.so.orig libfoo.O3.so.orig libfoo.Os.so.orig)

	foo_exe_varients=(foo.exe)
	libfoo_so_orig_varients=(libfoo.so.orig)
	libfoo_O_so_orig_varients=(libfoo.O.so.orig)
        libfoo_O2_so_orig_varients=(libfoo.O2.so.orig)
        libfoo_O3_so_orig_varients=(libfoo.O3.so.orig)
        libfoo_Os_so_orig_varients=(libfoo.Os.so.orig)
		
 	for file in "${files[@]}"; do
		for config in "${configs[@]}"; do
			echo Protecting file "$file" with config "$config"
			set -e
			"$config" ./"$file" ./"$file"".""$config"  > ps.log 2>&1 
			res=$?
			set +e
			if [[ $res  != 0 ]]; then
				echo "Failed to xform!"
				cat peasoup*/logs/*
				exit 1	
			else
				rm -Rf peasoup*
			fi
			
			varient_array_name="$(echo "$file" | sed -e 's/\./_/g')""_varients"
			varient_file="$file"".""$config"
                        eval $varient_array_name+=\(\$varient_file\)			
		done	
	done
	
}

clean()
{
	local ec=$?
	rm -f out >> /dev/null 2&>1
	rm -f correct >> /dev/null 2&>1
	rm -rf peasoup_executable_directory.* >> /dev/null 2&>1
	rm -f *.orig >> /dev/null 2&>1
	rm -f *.exe >> /dev/null 2&>1
	rm -f *.so >> /dev/null 2&>1
	
	for config in "${configs[@]}"; do
		rm -f *."$config" >> /dev/null 2&>1
	done
	
	exit $ec
}


main()
{
	build
	protect
	get_correct

	libfoo_varients=("${libfoo_so_orig_varients[@]}" "${libfoo_O_so_orig_varients[@]}" "${libfoo_O2_so_orig_varients[@]}"
                         "${libfoo_O3_so_orig_varients[@]}" "${libfoo_Os_so_orig_varients[@]}")

	
	for foo_varient in "${foo_exe_varients[@]}"; do
		for libfoo_varient in "${libfoo_varients[@]}"; do
			do_test "$foo_varient" "$libfoo_varient"
		done
	done	
	
	exit 0
}


main $*
