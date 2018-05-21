#!/bin/bash

#benchmarks="473.astar"
# 447.dealII // broken build
benchmarks=" 400.perlbench 401.bzip2 403.gcc 410.bwaves 416.gamess 429.mcf 433.milc 434.zeusmp 435.gromacs 436.cactusADM 437.leslie3d 444.namd 445.gobmk 450.soplex 453.povray 454.calculix 456.hmmer 458.sjeng 459.GemsFDTD 462.libquantum 464.h264ref 465.tonto 470.lbm 471.omnetpp 473.astar 481.wrf 482.sphinx3 483.xalancbmk "

#benchmarks="400.perlbench 403.gcc 445.gobmk 453.povray 458.sjeng 464.h264ref 464.tonto 471.omnetpp 481.wrf 482.sphinx3 483.xalancbmk"
number=1

setup()
{

	if [ ! -d spec2006 ]; then
		svn co ^/spec2006/trunk spec2006
	fi


	cd spec2006/
	if [ ! -L bin ]; then
		ln -s bin.power/ bin
	fi
	source shrc
	bin/relocate
}


run_test()
{
	local config_name=$1
	local config=$2

	all_configs_that_were_run="$all_configs_that_were_run $config_name"

	cd $SPEC
	if [ ! -d result.$config_name ]; then
		echo "Re-doing runspec for test $config_name"
		dropdb  $PGDATABASE 
		createdb  $PGDATABASE 
		$PEASOUP_HOME/tools/db/pdb_setup.sh
		rm -Rf result/*
		runspec  --action scrub --config $config $benchmarks
		runspec  --action validate --config $config -n $number $benchmarks 
		cp benchspec/CPU2006/*/exe/* result
		mv result result.$config_name
		dropdb  $PGDATABASE 
		createdb  $PGDATABASE 
		$PEASOUP_HOME/tools/db/pdb_setup.sh
	fi

}

get_size_result()
{
	bench=$1
	if [ -f $1 ]; then
		size=$(stat --printf="%s" $bench)
		echo -n $size
	else
		echo -n 0
	fi
}

get_result()
{
	bench=$1
	config=$2

	results=$(cat $SPEC/result.$config/CPU2006.002.log 2>/dev/null|grep Success|grep $bench|grep ratio=|sed 's/.*ratio=//'|sed 's/,.*//')

	sum=0
	count=0
	for res in $results
	do
		sum=$(echo $sum + $res | bc)
		count=$(echo $count + 1  | bc)
	done
	#echo sum=$sum
	#echo count=$count
	res=$(echo  "scale=2; $sum / $count" | bc 2> /dev/null )

	count=$(echo $res|wc -w)

	if [ $count = 1 ];  then
		echo -n $res
	else
		echo -n "N/A"
	fi

}


# global
all_configs_that_were_run=""

get_raw_results()
{
	echo "--------------------------------------------------------------"
	echo "Performance results are:"
	echo "--------------------------------------------------------------"
	local configs="$all_configs_that_were_run"
	echo benchmark $configs
	for bench in $benchmarks
	do
		echo -n "$bench 	"
		for config in $configs 
		do
			get_result $bench $config
			echo -n "	"
		done
		echo
	done

	echo "--------------------------------------------------------------"
	echo "Size results are:"
	echo "--------------------------------------------------------------"
	echo benchmark $configs
	for bench in $SPEC/result.$config/*_base.amd64-m64-gcc42-nn
	do
		echo -n "$(basename $bench _base.amd64-m64-gcc42-nn) 	"
		for config in $configs
		do
			file="$SPEC/result.$config/$(basename $bench)"
			get_size_result $file 
			echo -n " 	"
		done
		echo
	done

}


main()
{
	start_dir=$(pwd)
	setup


	local zipr_flags="--backend zipr"
	local mm_basic_cfi_flags="--step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--fix-all "
	local mm_basic_cfi_exe_nonces_flags="--step move_globals=on --step selective_cfi=on --step-option selective_cfi:--multimodule --step-option move_globals:--elftables-only  --step-option fix_calls:--no-fix-safefn --step-option selective_cfi:--exe-nonce-for-call "
	local mm_color_cfi_flags="$mm_basic_cfi_flags --step-option selective_cfi:--color"
	local trace_flags="-o zipr:--traceplacement:on -o zipr:true"
	

	# no $PS -- aka, baseline.
	run_test original $SPEC/config/ubuntu14.04lts-64bit.cfg

	# zipr, basic cfi, basic cfi with exe nonces for calls, color cfi
	PSOPTS="$zipr_flags"  				       run_test zipr                    $SPEC/config/ubuntu14.04lts-64bit-withps.cfg
	PSOPTS="$zipr_flags $mm_basic_cfi_flags " 	       run_test mm-basic-cfi            $SPEC/config/ubuntu14.04lts-64bit-withps.cfg
	PSOPTS="$zipr_flags $mm_basic_cfi_exe_nonces_flags "   run_test mm-basic-cfi-exe-nonces $SPEC/config/ubuntu14.04lts-64bit-withps.cfg
	PSOPTS="$zipr_flags $mm_color_cfi_flags" 	       run_test mm-color-cfi            $SPEC/config/ubuntu14.04lts-64bit-withps.cfg

	# zipr, basic cfi, color cfi
	# with trace placement
	PSOPTS="$zipr_flags $trace_flags "  					run_test zipr-trace         		$SPEC/config/ubuntu14.04lts-64bit-withps.cfg
	PSOPTS="$zipr_flags $mm_basic_cfi_flags  $trace_flags " 		run_test mm-basic-cfi-trace 		$SPEC/config/ubuntu14.04lts-64bit-withps.cfg
	PSOPTS="$zipr_flags $mm_basic_cfi_exe_nonces_flags $trace_flags "   	run_test mm-basic-cfi-exe-nonces-trace 	$SPEC/config/ubuntu14.04lts-64bit-withps.cfg
	PSOPTS="$zipr_flags $mm_color_cfi_flags $trace_flags " 			run_test mm-color-cfi-trace 		$SPEC/config/ubuntu14.04lts-64bit-withps.cfg

	get_raw_results 

}

main "$@"



