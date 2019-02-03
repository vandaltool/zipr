#!/bin/bash

source ../cfi_smokescreen_configs.sh 

get_correct()
{
	cp libfoo.so.orig libfoo.so
	./foo.exe > correct
}

test()
{
	
	cp $2 libfoo.so  
	./$1 > out 

	cmp out correct
	if [ $? = 1 ]; then
		fails=$(expr $fails + 1 )
		echo test "$1" "$2" failed | tee -a foo_test_log.txt
	else
		passes=$(expr $passes + 1 )
		echo test passed.
	fi
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
			echo Protecting file "$file" with config "$config" | tee -a foo_protection_log.txt
			"$config" ./"$file" ./"$file"".""$config" | tee -a foo_protection_log.txt
			varient_array_name="$(echo "$file" | sed -e 's/\./_/g')""_varients"
			declare -n varient_array="$varient_array_name"
			varient_array+=("$file"".""$config")
		done	
	done
	
}

clean()
{
	rm out
	rm correct
	rm -rf peasoup_executable_directory.*
	rm *.orig
	rm *.exe
	rm *.so
	
	for config in "${configs[@]}"; do
		rm *."$config"
	done
}

report ()
{
	total=$(expr $passes + $fails)
	echo "Passes:  $passes / $total" | tee -a foo_test_log.txt
	echo "Fails :  $fails / $total" | tee -a foo_test_log.txt
	
	if grep -q "Warning " ./foo_protection_log.txt
	then
		echo PROTECTION WARNINGS DETECTED!
	else
		echo ALL PROTECTIONS SUCCESSFUL
	fi
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
	
	report
	clean
}

passes=0 
fails=0

main $*
