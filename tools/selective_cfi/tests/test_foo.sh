#!/bin/bash 

do_cfi()
{
	$PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false"
}

# Note: exe nonce cfi doesn't always run against non-exe nonce cfi modules
do_cfi_exe_nonces()
{
        $PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false"
}

do_coloring_cfi()
{
	$PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only --step-option fix_calls:--fix-all --step-option selective_cfi:--color --step-option zipr:"--add-sections false"
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
	do_cfi_exe_nonces ./libfoo.so.orig ./libfoo.so.exe.nonce.cfi
	do_cfi_exe_nonces ./foo.exe ./foo.exe.nonce.cfi
	do_cfi ./foo.exe ./foo.exe.no-color.cfi
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

	test foo.exe.nonce.cfi libfoo.so.exe.nonce.cfi
	test foo.exe libfoo.so.exe.nonce.cfi
	report
#	clean
}

passes=0 
fails=0

main $*
