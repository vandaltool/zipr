#!/bin/bash  

set -e
trap clean EXIT

cd $CICD_MODULE_WORK_DIR/irdblibs_umbrella
source set_env_vars
cd ./irdb-libs/tools/selective_cfi/tests/cicd_tests/fib_src

source ../../cfi_smokescreen_configs.sh

get_correct()
{
	cp libfib.so.orig libfib.so
	cp libfib2.so.orig libfib2.so
	# for fib.exe, a non-zero exit status can indicate success
	set +e
	./fib.exe $1 > correct
	echo $? >> correct
	set -e
}

do_test()
{
	echo running test $1 $2 $3 $4
	
	n=$2

	get_correct $n

	cp $3 libfib.so  
	cp $4 libfib2.so 
	# for fib.exe, a non-zero exit status can indicate success
	set +e
	./$1 $n > out 
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
			set +e
                        "$config" ./"$file" ./"$file"".""$config" > out 2>&1 
			res=$?
			set -e
			if [[ $res == 0 ]]; then
				rm -rf out peasoup*
			else
				echo "Failed protection!"
				cat out peasoup*/logs/*
				exit 1
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
	rm -f out >> /dev/null 2>&1
	rm -f correct >> /dev/null 2>&1
	rm -Rf fib.exe* peasoup_exe* lib*.so* >> /dev/null 2>&1

 	for config in "${configs[@]}"; do
                rm -f *."$config" >> /dev/null 2>&1 
        done	
	exit $ec
}


main()
{
	build
	protect

	fib_varients=("${fib_exe_varients[@]}" "${fib_exe_pie_varients[@]}")

	for fib_varient in "${fib_varients[@]}"; do
                for libfib_varient in "${libfib_so_orig_varients[@]}"; do
			for libfib2_varient in "${libfib2_so_orig_varients[@]}"; do
				for i in 2 6; do
                        		do_test "$fib_varient" $i "$libfib_varient" "$libfib2_varient"
				done
			done
                done
        done
	
	exit 0	
}


main $*
