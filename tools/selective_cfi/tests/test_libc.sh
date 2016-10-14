#!/bin/bash 

do_cfi()
{
	$PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false"
	if [ ! $? -eq 0 ]; then
		echo "do_cfi(): failed to protect"
	fi
}

do_coloring_cfi()
{
	$PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option selective_cfi:--color  --step-option zipr:"--add-sections false"
	if [ ! $? -eq 0 ]; then
		echo "do_coloring_cfi(): failed to protect"
	fi
}


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
		echo test failed
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
	do_cfi libc.so.6.orig libc-2.19.so.cfi
}

clean()
{
	rm out
	rm correct
	rm -Rf foo.exe peasoup_exe* libfoo.so libfoo.so.orig libfoo.so.cfi foo.cfi foo.exe.cfi
}

report ()
{
	total=$(expr $passes + $fails)
	echo "Passes:  $passes / $total"
	echo "Fails :  $fails / $total"
}

main()
{
	build
	protect
#	get_correct
#	report
#	clean
}

passes=0 
fails=0

main $*
