#!/bin/bash  

do_cfi()
{
	$PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false"
}

do_coloring_cfi()
{
	$PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option selective_cfi:--color  --step-option zipr:"--add-sections false"
}


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
		echo test failed $1 $2 $3 $4
		echo "=== out ==="
		cat out
		echo "======"
		echo "exiting"
		#clean
		exit 1
	else
		passes=$(expr $passes + 1 )
		echo test passed.
	fi
}


build()
{
	gcc -o libfib.so libfib.c -w -shared -fPIC
	gcc -o libfib2.so libfib2.c -w -shared -fPIC
	gcc -o fib.exe fib.c -w -L. -lfib -lfib2
	gcc -o fib.exe.pie fib.c -fPIC -fpie -pie -w -L. -lfib -lfib2
	mv libfib.so libfib.so.orig
	mv libfib2.so libfib2.so.orig

	gcc -o libfib2.O.so libfib2.c -O -w -shared -fPIC
	gcc -o libfib2.O2.so libfib2.c -O2 -w -shared -fPIC
	gcc -o libfib2.O3.so libfib2.c -O3 -w -shared -fPIC
	gcc -o libfib2.Os.so libfib2.c -Os -w -shared -fPIC
	mv libfib2.O.so libfib2.O.so.orig
	mv libfib2.O2.so libfib2.O2.so.orig
	mv libfib2.O3.so libfib2.O3.so.orig
	mv libfib2.Os.so libfib2.Os.so.orig

	gcc -o libfib.O.so libfib.c -O -w -shared -fPIC
	gcc -o libfib.O2.so libfib.c -O2 -w -shared -fPIC
	gcc -o libfib.O3.so libfib.c -O3 -w -shared -fPIC
	gcc -o libfib.Os.so libfib.c -Os -w -shared -fPIC
	mv libfib.O.so libfib.O.so.orig
	mv libfib.O2.so libfib.O2.so.orig
	mv libfib.O3.so libfib.O3.so.orig
	mv libfib.Os.so libfib.Os.so.orig
}


protect()
{
	do_cfi ./fib.exe ./fib.exe.cfi
	do_cfi ./libfib.so.orig ./libfib.so.cfi
	do_cfi ./libfib2.so.orig ./libfib2.so.cfi

	do_coloring_cfi ./fib.exe ./fib.exe.cfi.color
	do_coloring_cfi ./libfib.so.orig ./libfib.so.cfi.color
	do_coloring_cfi ./libfib2.so.orig ./libfib2.so.cfi.color

	do_cfi ./libfib2.O.so.orig ./libfib2.O.so.cfi
	do_cfi ./libfib2.O2.so.orig ./libfib2.O2.so.cfi
	do_cfi ./libfib2.O3.so.orig ./libfib2.O3.so.cfi
	do_cfi ./libfib2.Os.so.orig ./libfib2.Os.so.cfi

	do_coloring_cfi ./libfib2.O.so.orig ./libfib2.O.so.cfi.color
	do_coloring_cfi ./libfib2.O2.so.orig ./libfib2.O2.so.cfi.color
	do_coloring_cfi ./libfib2.O3.so.orig ./libfib2.O3.so.cfi.color
	do_coloring_cfi ./libfib2.Os.so.orig ./libfib2.Os.so.cfi.color

	do_cfi ./libfib.O.so.orig ./libfib.O.so.cfi
	do_cfi ./libfib.O2.so.orig ./libfib.O2.so.cfi
	do_cfi ./libfib.O3.so.orig ./libfib.O3.so.cfi
	do_cfi ./libfib.Os.so.orig ./libfib.Os.so.cfi

	do_coloring_cfi ./libfib.O.so.orig ./libfib.O.so.cfi.color
	do_coloring_cfi ./libfib.O2.so.orig ./libfib.O2.so.cfi.color
	do_coloring_cfi ./libfib.O3.so.orig ./libfib.O3.so.cfi.color
	do_coloring_cfi ./libfib.Os.so.orig ./libfib.Os.so.cfi.color
}

clean()
{
	rm out
	rm correct
	rm -Rf fib.exe* peasoup_exe* lib*.so*
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

	test fib.exe 2 libfib.so.orig libfib2.so.orig
	test fib.exe 3 libfib.so.orig libfib2.so.orig
	test fib.exe 4 libfib.so.orig libfib2.so.orig
	test fib.exe 5 libfib.so.orig libfib2.so.orig
	test fib.exe 6 libfib.so.orig libfib2.so.orig

	test fib.exe 2 libfib.so.cfi libfib2.so.orig
	test fib.exe 3 libfib.so.cfi libfib2.so.orig
	test fib.exe 4 libfib.so.cfi libfib2.so.orig
	test fib.exe 5 libfib.so.cfi libfib2.so.orig
	test fib.exe 6 libfib.so.cfi libfib2.so.orig

	test fib.exe 2 libfib.so.orig libfib2.so.cfi
	test fib.exe 3 libfib.so.orig libfib2.so.cfi
	test fib.exe 4 libfib.so.orig libfib2.so.cfi
	test fib.exe 5 libfib.so.orig libfib2.so.cfi
	test fib.exe 6 libfib.so.orig libfib2.so.cfi

	test fib.exe 2 libfib.so.cfi libfib2.so.cfi
	test fib.exe 3 libfib.so.cfi libfib2.so.cfi
	test fib.exe 4 libfib.so.cfi libfib2.so.cfi
	test fib.exe 5 libfib.so.cfi libfib2.so.cfi
	test fib.exe 6 libfib.so.cfi libfib2.so.cfi

	test fib.exe.cfi 2 libfib.so.orig libfib2.so.orig
	test fib.exe.cfi 3 libfib.so.orig libfib2.so.orig
	test fib.exe.cfi 4 libfib.so.orig libfib2.so.orig
	test fib.exe.cfi 5 libfib.so.orig libfib2.so.orig
	test fib.exe.cfi 6 libfib.so.orig libfib2.so.orig

	test fib.exe.cfi 2 libfib.so.cfi libfib2.so.orig
	test fib.exe.cfi 3 libfib.so.cfi libfib2.so.orig
	test fib.exe.cfi 4 libfib.so.cfi libfib2.so.orig
	test fib.exe.cfi 5 libfib.so.cfi libfib2.so.orig
	test fib.exe.cfi 6 libfib.so.cfi libfib2.so.orig

	test fib.exe.cfi 2 libfib.so.orig libfib2.so.cfi
	test fib.exe.cfi 3 libfib.so.orig libfib2.so.cfi
	test fib.exe.cfi 4 libfib.so.orig libfib2.so.cfi
	test fib.exe.cfi 5 libfib.so.orig libfib2.so.cfi
	test fib.exe.cfi 6 libfib.so.orig libfib2.so.cfi

	test fib.exe.cfi 2 libfib.so.cfi libfib2.so.cfi
	test fib.exe.cfi 3 libfib.so.cfi libfib2.so.cfi
	test fib.exe.cfi 4 libfib.so.cfi libfib2.so.cfi
	test fib.exe.cfi 5 libfib.so.cfi libfib2.so.cfi
	test fib.exe.cfi 6 libfib.so.cfi libfib2.so.cfi

	test fib.exe.cfi.color 2 libfib.so.orig libfib2.so.orig
	test fib.exe.cfi.color 3 libfib.so.orig libfib2.so.orig
	test fib.exe.cfi.color 4 libfib.so.orig libfib2.so.orig
	test fib.exe.cfi.color 5 libfib.so.orig libfib2.so.orig
	test fib.exe.cfi.color 6 libfib.so.orig libfib2.so.orig

	test fib.exe.cfi.color 2 libfib.so.cfi libfib2.so.orig
	test fib.exe.cfi.color 3 libfib.so.cfi libfib2.so.orig
	test fib.exe.cfi.color 4 libfib.so.cfi libfib2.so.orig
	test fib.exe.cfi.color 5 libfib.so.cfi libfib2.so.orig
	test fib.exe.cfi.color 6 libfib.so.cfi libfib2.so.orig

	test fib.exe.cfi.color 2 libfib.so.orig libfib2.so.cfi
	test fib.exe.cfi.color 3 libfib.so.orig libfib2.so.cfi
	test fib.exe.cfi.color 4 libfib.so.orig libfib2.so.cfi
	test fib.exe.cfi.color 5 libfib.so.orig libfib2.so.cfi
	test fib.exe.cfi.color 6 libfib.so.orig libfib2.so.cfi

	test fib.exe.cfi.color 2 libfib.so.cfi libfib2.so.cfi
	test fib.exe.cfi.color 3 libfib.so.cfi libfib2.so.cfi
	test fib.exe.cfi.color 4 libfib.so.cfi libfib2.so.cfi
	test fib.exe.cfi.color 5 libfib.so.cfi libfib2.so.cfi
	test fib.exe.cfi.color 6 libfib.so.cfi libfib2.so.cfi

	test fib.exe 2 libfib.so.cfi.color libfib2.so.orig
	test fib.exe 3 libfib.so.cfi.color libfib2.so.orig
	test fib.exe 4 libfib.so.cfi.color libfib2.so.orig
	test fib.exe 5 libfib.so.cfi.color libfib2.so.orig
	test fib.exe 6 libfib.so.cfi.color libfib2.so.orig

	test fib.exe 2 libfib.so.orig libfib2.so.cfi.color
	test fib.exe 3 libfib.so.orig libfib2.so.cfi.color
	test fib.exe 4 libfib.so.orig libfib2.so.cfi.color
	test fib.exe 5 libfib.so.orig libfib2.so.cfi.color
	test fib.exe 6 libfib.so.orig libfib2.so.cfi.color

	test fib.exe 2 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe 3 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe 4 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe 5 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe 6 libfib.so.cfi.color libfib2.so.cfi.color

	test fib.exe.cfi 2 libfib.so.cfi.color libfib2.so.orig
	test fib.exe.cfi 3 libfib.so.cfi.color libfib2.so.orig
	test fib.exe.cfi 4 libfib.so.cfi.color libfib2.so.orig
	test fib.exe.cfi 5 libfib.so.cfi.color libfib2.so.orig
	test fib.exe.cfi 6 libfib.so.cfi.color libfib2.so.orig

	test fib.exe.cfi 2 libfib.so.orig libfib2.so.cfi.color
	test fib.exe.cfi 3 libfib.so.orig libfib2.so.cfi.color
	test fib.exe.cfi 4 libfib.so.orig libfib2.so.cfi.color
	test fib.exe.cfi 5 libfib.so.orig libfib2.so.cfi.color
	test fib.exe.cfi 6 libfib.so.orig libfib2.so.cfi.color

	test fib.exe.cfi 2 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe.cfi 3 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe.cfi 4 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe.cfi 5 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe.cfi 6 libfib.so.cfi.color libfib2.so.cfi.color

	test fib.exe.cfi.color 2 libfib.so.cfi.color libfib2.so.orig
	test fib.exe.cfi.color 3 libfib.so.cfi.color libfib2.so.orig
	test fib.exe.cfi.color 4 libfib.so.cfi.color libfib2.so.orig
	test fib.exe.cfi.color 5 libfib.so.cfi.color libfib2.so.orig
	test fib.exe.cfi.color 6 libfib.so.cfi.color libfib2.so.orig

	test fib.exe.cfi.color 2 libfib.so.orig libfib2.so.cfi.color
	test fib.exe.cfi.color 3 libfib.so.orig libfib2.so.cfi.color
	test fib.exe.cfi.color 4 libfib.so.orig libfib2.so.cfi.color
	test fib.exe.cfi.color 5 libfib.so.orig libfib2.so.cfi.color
	test fib.exe.cfi.color 6 libfib.so.orig libfib2.so.cfi.color

	test fib.exe.cfi.color 2 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe.cfi.color 3 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe.cfi.color 4 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe.cfi.color 5 libfib.so.cfi.color libfib2.so.cfi.color
	test fib.exe.cfi.color 6 libfib.so.cfi.color libfib2.so.cfi.color

	# some tests for different optimization levels
	test fib.exe.cfi.color 5 libfib.so.orig libfib2.O.so.cfi
	test fib.exe.cfi.color 5 libfib.so.orig libfib2.O2.so.cfi
	test fib.exe.cfi.color 5 libfib.so.orig libfib2.O3.so.cfi
	test fib.exe.cfi.color 5 libfib.so.orig libfib2.Os.so.cfi

	test fib.exe.cfi.color 5 libfib.so.cfi.color libfib2.O.so.cfi.color
	test fib.exe.cfi.color 5 libfib.so.cfi.color libfib2.O2.so.cfi.color
	test fib.exe.cfi.color 5 libfib.so.cfi.color libfib2.O3.so.cfi.color
	test fib.exe.cfi.color 5 libfib.so.cfi.color libfib2.Os.so.cfi.color

	test fib.exe.cfi.color 5 libfib.O3.so.orig libfib2.O.so.cfi
	test fib.exe.cfi.color 5 libfib.O2.so.orig libfib2.O2.so.cfi
	test fib.exe.cfi.color 5 libfib.Os.so.orig libfib2.O3.so.cfi
	test fib.exe.cfi.color 5 libfib.O.so.orig libfib2.Os.so.cfi

	test fib.exe.cfi.color 5 libfib.O2.so.cfi.color libfib2.O.so.cfi.color
	test fib.exe.cfi.color 5 libfib.O3.so.cfi.color libfib2.O2.so.cfi.color
	test fib.exe.cfi.color 5 libfib.O.so.cfi.color libfib2.O3.so.cfi.color
	test fib.exe.cfi.color 5 libfib.Os.so.cfi.color libfib2.Os.so.cfi.color

	report
	clean
}

passes=0 
fails=0

main $*
