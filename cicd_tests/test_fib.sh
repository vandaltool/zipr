#!/bin/bash  

set -x
set -e
trap clean EXIT

cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
source set_env_vars
cd ./security_transforms/cicd_tests

source cfi_smokescreen_configs.sh

get_correct()
{
	cp libfib.so.orig libfib.so
	cp libfib2.so.orig libfib2.so
	./fib.exe $1 > correct
	echo $? >> correct
}

test()
{
	echo running test $1 $2 $3 $4
	
	n=$2

	get_correct $n

	cp $3 libfib.so  
	cp $4 libfib2.so  
	./$1 $n > out 
	echo $? >> out

	cmp out correct
}


build()
{
	gcc -o libfib.so libfib.c -w -shared -fPIC
	gcc -o libfib2.so libfib2.c -w -shared -fPIC
	gcc -o fib.exe fib.c -w -L. -lfib -lfib2
	gcc -o fib.exe.pie fib.c -fPIC -fpie -pie -w -L. -lfib -lfib2
	mv libfib.so libfib.so.orig
	mv libfib2.so libfib2.so.orig
}


protect()
{
	files=(libfib.so.orig libfib2.so.orig fib.exe fib.exe.pie)

	libfib_so_orig_varients=(libfib.so.orig)
	libfib2_so_orig_varients=(libfib2.so.orig)
	fib_exe_varients=(fib.exe)
	fib_exe_pie_varients=(fib.exe.pie)	
 
        for file in "${files[@]}"; do
                for config in "${configs[@]}"; do
                        echo Protecting file "$file" with config "$config"
                        "$config" ./"$file" ./"$file"".""$config"
                        varient_array_name="$(echo "$file" | sed -e 's/\./_/g')""_varients"
                        declare -n varient_array="$varient_array_name"
                        varient_array+=("$file"".""$config")
                done
        done	
}

clean()
{
	rm out >> /dev/null
	rm correct >> /dev/null
	rm -Rf fib.exe* peasoup_exe* lib*.so* >> /dev/null

 	for config in "${configs[@]}"; do
                rm *."$config" >> /dev/null
        done	
}


main()
{
	build
	protect

	fib_varients=("${fib_exe_varients[@]}" "${fib_exe_pie_varients[@]}")

	for fib_varient in "${fib_varients[@]}"; do
                for libfib_varient in "${libfib_so_orig_varients[@]}"; do
			for libfib2_varient in "${libfib2_so_orig_varients[@]}"; do
				for i in {2...6}; do
                        		test "$fib_varient" $i "$libfib_varient" "$libfib2_varient"
				done
			done
                done
        done
	
	exit 0	
}


main $*
