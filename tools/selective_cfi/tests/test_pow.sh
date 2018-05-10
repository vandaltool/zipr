#!/bin/bash 

do_cfi()
{
	(set -x ; $PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false")
}

# Note: exe nonce cfi doesn't always run against non-exe nonce cfi modules
do_cfi_exe_nonces()
{
        $PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call --step-option zipr:"--add-sections false"
}

do_coloring_cfi()
{
	(set -x ; $PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--fix-all --step-option selective_cfi:--color  --step-option zipr:"--add-sections false" )
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
	
	local fail=0
	cp $lib libm.so.6  || fail=1
	./$exe $* > out 

	cmp out correct
	if [[ $? = 1 ]] || [[ $fail = 1 ]] ; then
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
	do_cfi_exe_nonces ./pow.exe ./pow.exe.nonce.cfi
	do_cfi_exe_nonces ./libm.so.6.orig ./libm.so.6.exe.nonce.cfi
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
	get_correct 3 5
        test pow.exe libm.so.6.orig     3 5     # unprotected - should pass! 3 5
        test pow.exe.nonce.cfi libm.so.6.orig 3 5     # main exe
        test pow.exe libm.so.6.exe.nonce.cfi      3 5     # shared lib only
        test pow.exe.nonce.cfi libm.so.6.exe.nonce.cfi  3 5     # both protected
        get_correct 5 8
        test pow.exe libm.so.6.orig     5 8     # unprotected - should pass! 3 5
        test pow.exe.nonce.cfi libm.so.6.orig 5 8     # main exe
        test pow.exe libm.so.6.exe.nonce.cfi      5 8     # shared lib only
        test pow.exe.nonce.cfi libm.so.6.exe.nonce.cfi  5 8     # both protected

	report
	if [[ ! $1 == -k ]]; then
		clean
	fi
}

passes=0 
fails=0

main $*
