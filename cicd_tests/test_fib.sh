#!/bin/bash  

cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
set -x
source ../tools/selective_cfi/tests/cfi_smokescreen_configs.sh

get_correct()
{
	cp libfib.so.orig libfib.so
	cp libfib2.so.orig libfib2.so
	./fib.exe $1 > correct
	echo $? >> correct
}

test()
{
	n=$2

	get_correct $n

	cp $3 libfib.so  
	cp $4 libfib2.so  
	./$1 $n > out 
	echo $? >> out

	cmp out correct
	if [ $? = 1 ]; then
		fails=$(expr $fails + 1 )
		echo test failed $1 $2 $3 $4 | tee -a fib_test_log.txt
		echo "=== out ==="
		cat out
		echo "======"
		echo "exiting"
		clean
		exit 1
	else
		passes=$(expr $passes + 1 )
		echo test passed.
	fi
}


build()
{
	gcc -o libfib.so ../tools/selective_cfi/tests/libfib.c -w -shared -fPIC
	gcc -o libfib2.so ../tools/selective_cfi/tests/libfib2.c -w -shared -fPIC
	gcc -o fib.exe ../tools/selective_cfi/tests/fib.c -w -L. -lfib -lfib2
	gcc -o fib.exe.pie ../tools/selective_cfi/tests/fib.c -fPIC -fpie -pie -w -L. -lfib -lfib2
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
                        echo Protecting file "$file" with config "$config" | tee -a fib_protection_log.txt
                        "$config" ./"$file" ./"$file"".""$config" | tee -a fib_protection_log.txt
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
	rm fib_protection_log.txt >> /dev/null
	rm -Rf fib.exe* peasoup_exe* lib*.so* >> /dev/null

 	for config in "${configs[@]}"; do
                rm *."$config" >> /dev/null
        done	
}

report ()
{
	total=$(expr $passes + $fails)
	echo "Passes:  $passes / $total" | tee -a fib_test_log.txt
	echo "Fails :  $fails / $total" | tee -a fib_test_log.txt

	if grep -q "Warning " ./fib_protection_log.txt
        then
                echo PROTECTION WARNINGS DETECTED!
		clean
		exit 1
        else
                echo ALL PROTECTIONS SUCCESSFUL
		clean
		exit 0
        fi
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
	
	report
}

passes=0 
fails=0

main $*
