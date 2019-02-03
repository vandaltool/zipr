#!/bin/bash

libcpath=""


source ../cfi_all_configs.sh


get_correct()
{
	echo "get_correct()"
	./libc_driver.exe $* > correct 
}

test()
{
	libc=$2
	pgm=$1
	shift 2

	LD_PRELOAD=$2 ./$pgm $* > out 

	cmp out correct
	if [ $? = 1 ]; then
		fails=$(expr $fails + 1 )
		echo test $pgm $libc failed | tee -a libc_test_log.txt
	else
		passes=$(expr $passes + 1 )
		echo test passed.
	fi
}


build()
{
	gcc -o libc_driver.exe libc_driver.c -w 

	libcpath=$(ldd libc_driver.exe|grep libc.so.6|sed "s/.*=>//"|sed "s/(.*//")
	echo libc=$libcpath
	cp $libcpath libc.so.6.orig
}


protect()
{
	files=(libc_driver.exe libc.so.6.orig)

        libc_driver_exe_varients=(libc_driver.exe)
        libc_so_6_orig_varients=(libc.so.6.orig)

        for file in "${files[@]}"; do
                for config in "${configs[@]}"; do
                        echo Protecting file "$file" with config "$config" | tee -a libc_protection_log.txt
                        "$config" ./"$file" ./"$file"".""$config" | tee -a libc_protection_log.txt
                        varient_array_name="$(echo "$file" | sed -e 's/\./_/g')""_varients"
			varient_file="$file"".""$config"
                        eval $varient_array_name+=\(\$varient_file\)                        
                done
        done	
}

clean()
{
	rm out
	rm correct
	rm -Rf *.orig *libc*.exe *libc.*cfi *peasoup_exec*

	for config in "${configs[@]}"; do
                rm *."$config"
        done
}

report ()
{
	total=$(expr $passes + $fails)
	echo "Passes:  $passes / $total" | tee -a libc_test_log.txt
	echo "Fails :  $fails / $total" | tee -a libc_test_log.txt
}

main()
{
	build
	protect
	get_correct

	for libc_driver_varient in "${libc_driver_exe_varients[@]}"; do
                for libc_varient in "${libc_so_6_orig_varients[@]}"; do
                        test "$libc_driver_varient" "$libc_varient"
                done
        done	

	report
	clean
}

passes=0 
fails=0

main $*
