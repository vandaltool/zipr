#!/bin/bash

set -x
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
}

test()
{
	echo Running test: "$1" "$2"
	
	cp $2 libfoo.so  
	./$1 > out 

	cmp out correct
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
			"$config" ./"$file" ./"$file"".""$config"
			varient_array_name="$(echo "$file" | sed -e 's/\./_/g')""_varients"
			varient_file="$file"".""$config"
                        eval $varient_array_name+=\(\$varient_file\)			
		done	
	done
	
}

clean()
{
	set +e
	set +x
	rm out >> /dev/null 2&>1
	rm correct >> /dev/null 2&>1
	rm -rf peasoup_executable_directory.* >> /dev/null 2&>1
	rm *.orig >> /dev/null 2&>1
	rm *.exe >> /dev/null 2&>1
	rm *.so >> /dev/null 2&>1
	
	for config in "${configs[@]}"; do
		rm *."$config" >> /dev/null 2&>1
	done
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
			test "$foo_varient" "$libfoo_varient"
		done
	done	
	
	exit 0
}


main $*
