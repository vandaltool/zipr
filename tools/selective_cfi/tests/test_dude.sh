#!/bin/bash  

source cfi_all_configs.sh

get_correct()
{
	cp libfoo.so.orig libfoo.so
	cp libdude.so.orig libdude.so
	./dude.exe > correct 2>&1 
}

test()
{
	local fail=0
	cp $2 libfoo.so   || fail=1
	cp $3 libdude.so   || fail=1
	./$1 > out  2>&1 

	cmp out correct
	if [[ $? = 1 ]] || [[ $fail = 1 ]] ; then
		fails=$(expr $fails + 1 )
		echo test failed $1 $2 $3 | tee -a dude_test_log.txt
		echo "=== out ==="
		cat out
		echo "======"
	else
		passes=$(expr $passes + 1 )
		echo test passed.
	fi
}


build()
{
	gcc -o libfoo.so libfoo.c -w -shared -fPIC
	gcc -o libdude.so libdude.c -w -shared -fPIC
	gcc -o dude.exe dude.c -w -L. -ldude -lfoo
	gcc -o dude.exe.pie dude.c -fPIC -fpie -pie -w -L. -ldude -lfoo
	mv libfoo.so libfoo.so.orig
	mv libdude.so libdude.so.orig
}


protect()
{
	files=(dude.exe dude.exe.pie libfoo.so.orig libdude.so.orig)

        dude_exe_varients=(dude.exe)
	dude_exe_pie_varients=(dude.exe.pie)
        libfoo_so_orig_varients=(libfoo.so.orig)
	libdude_so_orig_varients=(libdude.so.orig)

        for file in "${files[@]}"; do
                for config in "${configs[@]}"; do
                        echo Protecting file "$file" with config "$config" | tee -a dude_protection_log.txt
                        "$config" ./"$file" ./"$file"".""$config" | tee -a dude_protection_log.txt
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
	rm -Rf dude.exe* peasoup_exe* lib*.so*

	for config in "${configs[@]}"; do
                rm *."$config"
        done
}

report ()
{
	total=$(expr $passes + $fails)
	echo "Passes:  $passes / $total" | tee -a dude_test_log.txt
	echo "Fails :  $fails / $total" | tee -a dude_test_log.txt

	if grep -q "Warning " ./dude_protection_log.txt
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

	dude_varients=("${dude_exe_varients[@]}" "${dude_exe_pie_varients[@]}")
	
	for dude_varient in "${dude_varients[@]}"; do
                for libfoo_varient in "${libfoo_so_orig_varients[@]}"; do
                        for libdude_varient in "${libdude_so_orig_varients[@]}"; do
                                test "$dude_varient" "$libfoo_varient" "$libdude_varient"
                        done
                done
        done	

	
	report
	if [[ $1 == "-k" ]] ; then
		echo "Skipping cleanup"
	else
		clean
	fi
}

passes=0 
fails=0

main $*

