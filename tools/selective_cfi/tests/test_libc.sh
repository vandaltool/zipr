#!/bin/bash

libcpath=""

do_cfi()
{
	$PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false"
	if [ ! $? -eq 0 ]; then
		echo "do_cfi(): failed to protect"
	fi
}

do_cfi_color()
{
	$PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option selective_cfi:--color  --step-option zipr:"--add-sections false"
	if [ ! $? -eq 0 ]; then
		echo "do_coloring_cfi(): failed to protect"
	fi
}


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
	do_cfi libc_driver.exe libc_driver.exe.cfi
	do_cfi_color libc.so.6.orig libc-2.19.so.colorcfi
	do_cfi_color libc_driver.exe libc_driver.exe.colorcfi
}

clean()
{
	rm out
	rm correct
	rm -Rf *.orig *libc*.exe *libc.*cfi *peasoup_exec*
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
	get_correct
	test libc_driver.exe 		libc.so.6.orig
	test libc_driver.exe.cfi 	libc.so.6.orig
	test libc_driver.exe.colorcfi 	libc.so.6.orig
	test libc_driver.exe 		libc.so.6.cfi
	test libc_driver.exe.cfi 	libc.so.6.cfi
	test libc_driver.exe.colorcfi 	libc.so.6.cfi
	test libc_driver.exe		libc.so.6.colorcfi
	test libc_driver.exe.cfi	libc.so.6.colorcfi
	test libc_driver.exe.colorcfi	libc.so.6.colorcfi

	report
	clean
}

passes=0 
fails=0

main $*
