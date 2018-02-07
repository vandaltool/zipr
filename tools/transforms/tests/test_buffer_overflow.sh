#!/bin/bash  

do_p1()
{
	if [[ -f $2 ]]; then
		echo "Eliding rebuild of $2"
	else
		if [ -z "$3" ]; then
			$PSZ $1 $2 --step p1transform=on 
		else
			$PSZ $1 $2 --step p1transform=on --step-option p1transform:"$3"
		fi
	fi
}

get_correct()
{
	./test_buffer_overflow.exe > correct
	./test_buffer_overflow.exe abc >> correct
}

test_functional()
{
	./$1 > out 
	./$1 abc >> out 

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

test_detection()
{
	./$1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	exitcode=$?
	if [ $exitcode -eq $2 ]; then
		passes=$(expr $passes + 1 )
		echo test passed.
	else
		fails=$(expr $fails + 1 )
		echo "test failed: does not detect overflow or wrong exit code: $1 (expected $2, got $exitcode)" 
	fi
}

build()
{
	gcc -o test_buffer_overflow.exe test_buffer_overflow.c  -fno-stack-protector
}


protect()
{
	do_p1 ./test_buffer_overflow.exe test_buffer_overflow.exe.p1.189 
	do_p1 ./test_buffer_overflow.exe test_buffer_overflow.exe.p1.188 "--detection_policy exit --detection_exit_code 188"
	do_p1 ./test_buffer_overflow.exe test_buffer_overflow.exe.p1.hlt "--detection_policy halt"
}

clean()
{
	rm out 2>/dev/null
	rm correct 2>/dev/null
	rm -Rf test_buffer_overflow.exe* peasoup_exe*  2>/dev/null
}

report ()
{
	total=$(expr $passes + $fails)
	echo "Passes:  $passes / $total"
	echo "Fails :  $fails / $total"
}

main()
{
	clean
	build
	protect
	get_correct

	echo "Test functionality"
	test_functional test_buffer_overflow.exe # unprotected - should pass!
	test_functional test_buffer_overflow.exe.p1.189 
	test_functional test_buffer_overflow.exe.p1.188 
	test_functional test_buffer_overflow.exe.p1.hlt 

	report

	echo "Test detection"
	passes=0 
	fails=0
	test_detection test_buffer_overflow.exe.p1.189 189
	test_detection test_buffer_overflow.exe.p1.188 188
	test_detection test_buffer_overflow.exe.p1.hlt 139

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
