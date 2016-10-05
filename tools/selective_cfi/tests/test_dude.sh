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
	cp libfoo.so.orig libfoo.so
	cp libdude.so.orig libdude.so
	./dude.exe > correct
}

test()
{
	cp $2 libfoo.so  
	cp $3 libdude.so  
	./$1 > out 

	cmp out correct
	if [ $? = 1 ]; then
		fails=$(expr $fails + 1 )
		echo test failed $1 $2 $3
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
	mv libfoo.so libfoo.so.orig
	mv libdude.so libdude.so.orig
}


protect()
{
	do_coloring_cfi ./dude.exe ./dude.exe.cfi
	do_cfi ./libfoo.so.orig ./libfoo.so.cfi
	do_cfi ./libdude.so.orig ./libdude.so.cfi
}

clean()
{
	rm out
	rm correct
	rm -Rf dude.exe peasoup_exe* libfoo.so libfoo.so.orig libfoo.so.cfi libdude.so.* foo.cfi dude.exe.cfi
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

#	test dude.exe libfoo.so.orig libdude.so.orig		# unprotected - should pass!
#	test dude.exe libfoo.so.cfi libdude.so.orig		
#	test dude.exe libfoo.so.orig libdude.so.cfi		
#	test dude.exe.cfi libfoo.so.orig libdude.so.orig	
#	test dude.exe.cfi libfoo.so.cfi libdude.so.orig		
#	test dude.exe.cfi libfoo.so.orig libdude.so.cfi		

	test dude.exe libfoo.so.cfi libdude.so.cfi		
#	test dude.exe.cfi libfoo.so.cfi libdude.so.cfi		

	report
#	clean
}

passes=0 
fails=0

main $*
