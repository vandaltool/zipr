#!/bin/bash

check_fail()
{
	res=$3
	if [ $res != 0 ]; 
	then 
		echo
		echo "Failed trying to protect $1"
		cat $2.ps_output
		failures="$1 $2 " $'\n' "$failures"
	fi
}

do_nocfi()
{
	if [ -e $2 ]; then
		echo -n "$(basename $2) exists ... "
		return
	fi
	echo -n "$(basename $2) ... "
        $PS $1 $2 --backend zipr > $2.ps_output 2>&1
	check_fail $1 $2 $?
}

do_cfi()
{
	if [ -e $2 ]; then
		echo -n "$(basename $2) exists ... "
		return
	fi
	echo -n "$(basename $2) ... "
        $PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option zipr:"--add-sections false" > $2.ps_output 2>&1
	check_fail $1 $2 $?
}

do_coloring_cfi()
{
	if [ -e $2 ]; then
		echo -n "$(basename $2) exists ... "
		return
	fi
	echo -n "$(basename $2) ... "
        $PS $1 $2 --backend zipr --step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--cfi  --step-option fix_calls:--fix-all --step-option selective_cfi:--color  --step-option zipr:"--add-sections false"> $2.ps_output 2>&1
	check_fail $1 $2 $?

}


get_source()
{
	echo -n "Getting coreutils source  ... "
	utils_dir=$(ls -d coreutils-*.*/ 2> /dev/null)
	if [ ! -d "$utils_dir" ]; then
		apt-get source coreutils
	fi
	utils_dir=$(ls -d coreutils-*.*/)

	if [ ! -d "$utils_dir" ]; then
		echo could/locate coreutils source
		exit 1
	fi
	echo "done!"
}

build_source()
{
	echo -n "Building coreutils source  ... "
	cd $utils_dir
	if [ ! -f Makefile ]; then
		./configure
	fi

	make > coreutils.make.out
	res=$?

	if [ $res != 0 ]; then
		echo "Coreutils Build Failed!"
	fi

	cd $start_dir

	echo "done!"
	
}

analyze()
{
	echo -n "Finding executables .... "
	candidate_exes=$(find $utils_dir/src -executable )
	for candidate_exe in $candidate_exes
	do
		if [ -d "$candidate_exe" ]; then 
			#echo skipping dir $candidate_exe
			continue;
		fi
		if [[ "$candidate_exe" == *.orig || "$candidate_exe" == *cfi ]]; then
			#echo skipping orig exe $candidate_exe
			continue;
		fi
		file $candidate_exe |grep ELF > /dev/null 2>&1 
		res=${PIPESTATUS[1]}

		if [ $res != 0 ]; then
			continue;
		fi

		exes="$exes $candidate_exe"

		load_sec_no=$(readelf -l $candidate_exe|grep LOAD|wc -l)

		# save/verify .orig version already exists.
		if  [ $load_sec_no == 2 ]; then
			# save .orig
			cp $candidate_exe $candidate_exe.orig
		else
			# ensure .orig already exists.
			if [ ! -x $candidate_exe.orig ]; then
				echo "Missing $candidate_exe!  Cannot continue ...."
				exit 1
			fi
		fi
	done
	echo "Done!"
	echo "Found $(echo $exes |wc -w) coreutil executables"
}

protect()
{
	for exe in $exes
	do
		echo -n "Protecting $(basename $exe) ... "
		do_nocfi $exe.orig $exe.nocfi
		do_cfi $exe.orig $exe.cfi
		do_coloring_cfi $exe.orig $exe.colorcfi
		echo "Done!"
	done
}

install_set()
{
	ext=$1
	for exe in $exes
	do
		if [ ! $exe.$ext ]; then echo "Cannot test $exe.ext -- Missing file!"; fi
		cp $exe.$ext $exe
	done
}

test_coreutils()
{
	ext=$1
	outfile=results.$ext.txt
	if [ ! -f $outfile ]; then
		install_set $ext
		cd $utils_dir
		echo -n "Testing $ext ... "
		SECONDS=0
		make check > ../$outfile 2>&1 
		duration=$SECONDS
		echo "Elapsed time: $(($duration / 60)) minutes and $(($duration % 60)) "\
			"seconds ($duration seconds total)." >> ../$outfile
		
		echo "Done!"
		cd $start_dir
	fi

	echo
	echo Results for $ext
	tail -100 $outfile | sed -n  '/==================/,/=====================/p'| sed "s/^/	/"
	tail -1 $outfile| sed "s/^/	/"

}

clean()
{
	if [ -d $utils_dir ]; then
		rm -Rf  $utils_dir
	fi
	
}

main()
{
	start_dir=$(pwd)
	get_source
	build_source
	analyze
	install_set orig
	protect
	test_coreutils orig
	test_coreutils nocfi
	test_coreutils cfi
	test_coreutils colorcfi


	echo "Protection failures:  $failures"
}

main "$@"

