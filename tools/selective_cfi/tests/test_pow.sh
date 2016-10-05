#!/bin/bash 

do_cfi()
{
	$PS $1 $2 --backend zipr --step selective_cfi=on --step-option selective_cfi:--multimodule --step move_globals=on --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false"
}

do_coloring_cfi()
{
	$PS $1 $2 --backend zipr --step selective_cfi=on --step-option selective_cfi:--multimodule --step move_globals=on --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option selective_cfi:--color  --step-option zipr:"--add-sections false"
}


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
	
	cp $lib libm.so.6  
	./$exe $* > out 

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
	gcc -o pow.exe pow.c -w -lm

	# get libm.so from system.
	#libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007fe0fd24b000)
	libmpath=$(ldd pow.exe|grep libm.so.6|sed "s/.*=>//"|sed "s/(.*//")
	echo libm=$libmpath
	cp $libmpath libm.so.6.orig

}


protect()
{
	do_coloring_cfi ./pow.exe ./pow.exe.cfi
	do_cfi ./libm.so.6.orig ./libm.so.6.cfi
}

clean()
{
	rm out
	rm correct
	rm -Rf pow.exe peasoup_exe* libm.so.6 libm.so.6.orig libm.so.6.cfi pow.cfi pow.exe.cfi
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
	get_correct 3 5
	test pow.exe libm.so.6.orig 	3 5	# unprotected - should pass! 3 5
	test pow.exe.cfi libm.so.6.orig	3 5	# main exe
	test pow.exe libm.so.6.cfi	3 5	# shared lib only
	test pow.exe.cfi libm.so.6.cfi	3 5	# both protected
	get_correct 5 8
	test pow.exe libm.so.6.orig 	5 8	# unprotected - should pass! 3 5
	test pow.exe.cfi libm.so.6.orig	5 8	# main exe
	test pow.exe libm.so.6.cfi	5 8	# shared lib only
	test pow.exe.cfi libm.so.6.cfi	5 8	# both protected
	report
#	clean
}

passes=0 
fails=0

main $*
