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

# all
all_benchmarks="400.perlbench 401.bzip2 403.gcc 410.bwaves 416.gamess 429.mcf 433.milc 434.zeusmp 435.gromacs 436.cactusADM 437.leslie3d 444.namd 445.gobmk 450.soplex 453.povray 454.calculix 456.hmmer 458.sjeng 459.GemsFDTD 462.libquantum 464.h264ref 465.tonto 470.lbm 471.omnetpp 473.astar 481.wrf 482.sphinx3 483.xalancbmk"


number=1

setup()
{

	if [ ! -d spec2006 ]; then
		svn co ^/spec2006/trunk spec2006
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
		runspec  --action validate --config $config -n $number $benchmarks 
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
	get_raw_perf_results "$@"
	get_raw_size_results "$@"
	get_raw_fde_results "$@"
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
	echo benchmark $configs
	for bench in $SPEC/result.$first_config/*_base.amd64-m64-gcc42-nn
	do
		echo -n "$(basename $bench _base.amd64-m64-gcc42-nn)	"
		for config in $*
		do
			if [[ $config == "baseline" ]]; then
				file="$SPEC/result.$config/$(basename $bench)"
				cp $file /tmp/foo.exe
				strip /tmp/foo.exe
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
	trace_flags="   --step-option zipr:--traceplacement:on --step-option zipr:true"
	relax_flags="   --step-option zipr:--relax:on --step-option zipr:true --step-option zipr:--unpin:on --step-option zipr:false"
	nounpin_flags=" --step-option zipr:--unpin:on --step-option zipr:false"
	split_flags="   --step-option fill_in_indtargs:--split-eh-frame "
	icall_flags="   --step-option fix_calls:--no-fix-icalls "
	p1flags=" 	--step p1transform=on " 
	start_dir=$(pwd)
	setup
	run_test baseline $SPEC/config/ubuntu14.04lts-64bit.cfg "$all_benchmarks"

	# should be 100% success, tested by jdh on 8/28/17 as 100% success.
	#PSOPTS="$zipr_flags "  run_test zipr     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"

	#PSOPTS="$zipr_flags $trace_flags "  run_test zipr-trace     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $split_flags "  run_test split     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $split_flags $icall_flags "  run_test split-no-fix-icalls     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $split_flags $icall_flags $trace_flags "  run_test split-no-fix-icalls-trace     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"

	# should be 100% success, tested by jdh on 8/28/17 as 100% success.
	PSOPTS="$zipr_flags $split_flags $trace_flags "  run_test split-trace     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"

	# p1 -- expect 2 failures (povray and omnetpp)
	#PSOPTS="$zipr_flags $trace_flags $p1flags "  run_test zipr-trace-p1     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $split_flags $trace_flags $p1flags "  run_test split-trace-p1     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"

	# p1 -- expect no failures (--split-eh-frame fixes povray and omnetpp)
	PSOPTS="$zipr_flags $split_flags $trace_flags $icall_flags $p1flags "  run_test split-no-fix-icalls-trace-p1     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"

	#PSOPTS="$zipr_flags $nounpin_flags "  run_test zipr-nounpin     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $relax_flags "  run_test zipr-relax     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $relax_flags $nounpin_flags "  run_test zipr-relax-nounpin     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $split_flags $nounpin_flags "  run_test split-nounpin     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $split_flags $relax_flags "  run_test split-relax     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $split_flags $icall_flags $relax_flags "  run_test split-no-fix-icalls-relax     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"
	#PSOPTS="$zipr_flags $split_flags $icall_flags $relax_flags $nounpin_flags "  run_test split-no-fix-icalls-relax-nounpin     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$all_benchmarks"


	#p1_broken_benchmarks="403.gcc 416.gamess 483.xalancbmk"
	#p1_broken_benchmarks="483.xalancbmk"
	#PN_NUMFUNCSTOTRY=9805 PSOPTS="$zipr_flags $trace_flags $p1flags "  run_test zipr-trace-p1-rerun     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg "$p1_broken_benchmarks"

	get_raw_results baseline  split-trace split-no-fix-icalls-trace-p1
	# get_raw_results baseline  zipr zipr-trace split-no-fix-icalls split-no-fix-icalls-trace  zipr-trace-p1 split-trace-p1 split-no-fix-icalls-trace-p1 zipr-trace-p1-rerun
}

main "$@"



