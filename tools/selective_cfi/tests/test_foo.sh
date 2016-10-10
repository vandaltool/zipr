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
	gcc -o libfoo.so libfoo.c -w -shared -fPIC
	gcc -o foo.exe foo.c -w -L. -lfoo
	mv libfoo.so libfoo.so.orig
}


protect()
{
	do_coloring_cfi ./foo.exe ./foo.exe.cfi
	do_cfi ./libfoo.so.orig ./libfoo.so.cfi
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
	get_correct
	test foo.exe libfoo.so.orig 		# unprotected - should pass!
	test foo.exe.cfi libfoo.so.orig		# main exe
	test foo.exe libfoo.so.cfi		# shared lib only
	test foo.exe.cfi libfoo.so.cfi		# both protected
	report
	clean
}

passes=0 
fails=0

main $*
