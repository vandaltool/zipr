#!/bin/bash

# the bad boys
#benchmarks="
#	400.perlbench
#	403.gcc
#	445.gobmk
#	450.soplex
#	453.povray
#	458.sjeng
#	464.h264ref
#	465.tonto
#	471.omnetpp
#	481.wrf
#	482.sphinx3
#	483.xalancbmk
#	"

# broken: 447.dealII

cint_benchmarks="400.perlbench 401.bzip2 403.gcc 429.mcf 445.gobmk 456.hmmer 458.sjeng 462.libquantum 464.h264ref 471.omnetpp 473.astar 483.xalancbmk"

c_cpp_benchmarks="$cint_benchmarks 433.milc 444.namd 450.soplex 453.povray 470.lbm 482.sphinx3"
fortran_benchmarks="410.bwaves 416.gamess 434.zeusmp 437.leslie3d 459.gemsFDTD 465.tonto"
mixed_c_fortran_benchnmarks="435.gromacs 436.cactusADM 454.calculix 481.wrf"

cfp_benchmarks="$fortran_benchmakrs $mixed_c_fortran_benchmarks 433.milc 444.namd 450.soplex 453.povray 470.lbm 482.sphinx3"

# all
#all_benchmarks="400.perlbench 401.bzip2 403.gcc 410.bwaves 416.gamess 429.mcf 433.milc 434.zeusmp 435.gromacs 436.cactusADM 437.leslie3d 444.namd 445.gobmk 450.soplex 453.povray 454.calculix 456.hmmer 458.sjeng 459.GemsFDTD 462.libquantum 464.h264ref 465.tonto 470.lbm 471.omnetpp 473.astar 481.wrf 482.sphinx3 483.xalancbmk"
#all_benchmarks=" 401.bzip2 "

all_benchmarks=" $c_cpp_benchmarks "

number=1
#input_size=test
input_size=ref

setup()
{

	if [ ! -d spec2006 ]; then
		#svn co ^/spec2006/trunk spec2006
		git clone --depth 1 git@git.zephyr-software.com:allzp/spec2006.git spec2006
	fi

	if [[ ! -f /usr/bin/gfortran ]]; then
		sudo apt-get install gfortran -y
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
	config_name=$1
	config=$2
	benchmarks="$3"
	global_config_list="$global_config_list $config_name"
	cd $SPEC
	if [ ! -d result.$config_name ]; then
		dropdb $PGDATABASE
		createdb $PGDATABASE
		$PEASOUP_HOME/tools/db/pdb_setup.sh
		rm -Rf result/*
		runspec  --action scrub --config $config $benchmarks

		echo
		echo "**************************************************************************"
		echo "Starting test of $config_name"
		echo "**************************************************************************"
		echo
		runspec  --action validate --config $config -n $number $benchmarks  -i $input_size
		cp benchspec/CPU2006/*/exe/* result
		mv result result.$config_name
		for bench in $benchmarks
		do
			mv benchspec/CPU2006/$bench/run/build*/peasoup*/logs result.$config_name/$bench.log
		done
	fi

}

get_size_result()
{
	bench=$1
	if [ -e $bench ]; then
		size=$(stat --printf="%s" $bench)
		#echo -n "$size"
		#LC_ALL= numfmt --grouping $size
		#LC_ALL= printf "%'d" $size
		#LC_NUMERIC=en_US printf "%'d" $size
		#LC_NUMERIC=en_US printf "%'f" $size
		#LC_NUMERIC=en_US printf "%'.f" $size
		#LC_NUMERIC=en_US printf "%'10.10f" $size
		#LC_NUMERIC=en_US /usr/bin/printf "%'d" $size
		echo $size
	else
		echo -n "0"
	fi
}

get_result()
{
	bench=$1
	config=$2

	results=$(cat $SPEC/result.$config/CPU2006.002.log|grep Success|grep $bench|grep ratio=|sed 's/.*ratio=//'|sed 's/,.*//')

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
		echo -n "0"
	fi

}


get_raw_results()
{
	local configs="$global_config_list"
	get_raw_perf_results $configs
	get_raw_size_results $configs
	#get_raw_fde_results $configs
}

get_raw_perf_results()
{
	configs=$*
	first_config=$1
	echo "--------------------------------------------------------------"
	echo "Performance results are:"
	echo "--------------------------------------------------------------"
	echo benchmark $configs
	for bench in $benchmarks
	do
		echo -n "$bench 	"
		for config in $*
		do
			get_result $bench $config
			echo -n "	"
		done
		echo
	done
}

get_raw_size_results()
{
	echo "--------------------------------------------------------------"
	echo "Size results are:"
	echo "--------------------------------------------------------------"
	configs=$*
	first_config=$1
	echo benchmark $configs
	for bench in $SPEC/result.$first_config/*_base.amd64-m64-gcc42-nn
	do
		echo -n "$(basename $bench _base.amd64-m64-gcc42-nn)	"
		for config in $*
		do
			if [[ $config == "baseline" ]]; then
				file="$SPEC/result.$config/$(basename $bench)"
				cp $file /tmp/foo.exe
				strip /tmp/foo.exe 2> /dev/null
				file="/tmp/foo.exe"
			else
				file="$SPEC/result.$config/$(basename $bench)"
			fi
			res=$(get_size_result $file)

			#printf "%15s" $res
			echo -n "	$res"
		done
		echo
	done

}

get_raw_fde_results()
{
	echo "--------------------------------------------------------------"
	echo "FDE results are:"
	echo "--------------------------------------------------------------"
	configs=$*
	first_config=$1
	echo benchmark $configs
	for bench in $SPEC/result.$first_config/*_base.amd64-m64-gcc42-nn
	do
		#printf "%-20s"  $(basename $bench _base.amd64-m64-gcc42-nn)
		echo -n $(basename $bench _base.amd64-m64-gcc42-nn)
		for config in $*
		do
			file="$SPEC/result.$config/$(basename $bench)"
			res=$(readelf -w $file |grep FDE|wc -l )
			#if [[ $config == "baseline" ]]; then
			#else
			#fi

			#printf "%15s" $res
			echo -n "	$res"
		done
		echo
	done

}

main()
{
	zipr_flags="	--backend zipr --step-option zipr:--add-sections --step-option zipr:true"
	rida_flags="	$zipr_flags -c rida=on -s meds_static=off "
	trace_flags="   --step-option zipr:--traceplacement:on --step-option zipr:true"
	relax_flags="   --step-option zipr:--relax:on --step-option zipr:true --step-option zipr:--unpin:on --step-option zipr:false"
	nounpin_flags=" --step-option zipr:--unpin:on --step-option zipr:false"
	split_flags="   --step-option fill_in_indtargs:--split-eh-frame "
	icall_flags="   --step-option fix_calls:--no-fix-icalls "
	p1flags=" 	-c p1transform=on " 
	siflags=" 	$p1flags -s initialize_stack=on " 
        ssflags="       -c stack_stamp=on "
	mga_flags="      -c move_globals=on -o move_globals:--aggressive "
	mgc_flags="      -c move_globals=on -o move_globals:--conservative "

        nosflags="      -c non_overlapping_stack=on "
	start_dir=$(pwd)
	setup

	# baseline 
	run_test baseline $SPEC/config/ubuntu14.04lts-64bit.cfg "$all_benchmarks"

	PSOPTS="$zipr_flags "  run_test zipr     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"

	get_raw_results 
}

main "$@"



