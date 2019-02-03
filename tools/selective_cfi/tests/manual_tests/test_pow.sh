#!/bin/bash 


source ../cfi_all_configs.sh


get_correct()
{
	cp libm.so.6.orig libm.so.6
	./pow.exe $* > correct
}

test()
{
	exe=$1
	shift
	lib=$1
	shift

	get_correct $1 $2
	
	local fail=0
	cp $lib libm.so.6  || fail=1
	./$exe $* > out 

	cmp out correct
	if [[ $? = 1 ]] || [[ $fail = 1 ]] ; then
		fails=$(expr $fails + 1 )
		echo test $exe $lib $1 $2 failed | tee -a pow_test_log.txt
	else
		passes=$(expr $passes + 1 )
		echo test passed.
	fi
}


build()
{
	gcc -o pow.exe pow.c -w -lm

	# get libm.so from system.
	#libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007fe0fd24b000)
	libmpath=$(ldd pow.exe|grep libm.so.6|sed "s/.*=>//"|sed "s/(.*//")
	echo libm=$libmpath
	cp $libmpath libm.so.6.orig

}


protect()
{
	files=(pow.exe libm.so.6.orig)

        pow_exe_varients=(pow.exe)
        libm_so_6_orig_varients=(libm.so.6.orig)
                
        for file in "${files[@]}"; do
                for config in "${configs[@]}"; do
                        echo Protecting file "$file" with config "$config" | tee -a pow_protection_log.txt
                        "$config" ./"$file" ./"$file"".""$config" | tee -a pow_protection_log.txt
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
	rm -Rf pow.exe peasoup_exe* libm.so.6 libm.so.6.orig libm.so.6.cfi pow.cfi pow.exe.cfi

	for config in "${configs[@]}"; do
                rm *."$config"
        done
}

report ()
{
	total=$(expr $passes + $fails)
	echo "Passes:  $passes / $total" | tee -a pow_test_log.txt
	echo "Fails :  $fails / $total" | tee -a pow_test_log.txt
}

main()
{
	build
	protect

	
        for pow_varient in "${pow_exe_varients[@]}"; do
                for libm_varient in "${libm_so_6_orig_varients[@]}"; do
                        test "$pow_varient" "$libm_varient" 3 5
			test "$pow_varient" "$libm_varient" 5 8
                done
        done
	
	report
	if [[ ! $1 == -k ]]; then
		clean
	fi
}

passes=0 
fails=0

main $*
