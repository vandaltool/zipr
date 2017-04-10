#!/bin/bash

benchmarks="
	400.perlbench
	403.gcc
	445.gobmk
	450.soplex
	453.povray
	458.sjeng
	464.h264ref
	465.tonto
	471.omnetpp
	481.wrf
	482.sphinx3
	483.xalancbmk
	"
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
	config_name=$1
	config=$2
	cd $SPEC
	if [ ! -d result.$config_name ]; then
		rm -Rf result/*
		runspec  --action scrub --config $config $benchmarks
		runspec  --action validate --config $config -n $number $benchmarks 
		cp benchspec/CPU2006/*/exe/* result
		mv result result.$config_name
	fi

}

get_size_result()
{
	bench=$1
	size=$(stat --printf="%s" $bench)
	echo -n $size
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
		echo -n "N/A"
	fi

}

get_raw_results()
{
	echo "--------------------------------------------------------------"
	echo "Performance results are:"
	echo "--------------------------------------------------------------"
	configs=$*
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

	echo "--------------------------------------------------------------"
	echo "Size results are:"
	echo "--------------------------------------------------------------"
	configs=$*
	echo benchmark $configs
	for bench in $SPEC/result.$config/*_base.amd64-m64-gcc42-nn
	do
		echo -n "$(basename $bench _base.amd64-m64-gcc42-nn) 	"
		for config in $*
		do
			file="$SPEC/result.$config/$(basename $bench)"
			get_size_result $file 
			echo -n "	"
		done
		echo
	done

}


main()
{
	start_dir=$(pwd)
	setup
	run_test baseline $SPEC/config/ubuntu14.04lts-64bit.cfg
	PSOPTS="--backend zipr"  run_test zipr     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg
	PSOPTS="--backend zipr --step-option fill_in_indtargs:--split-eh-frame "  run_test split     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg
	PSOPTS="--backend zipr --step-option fill_in_indtargs:--split-eh-frame --step-option fix_calls:--no-fix-icalls "  run_test split-no-fix-icalls     $SPEC/config/ubuntu14.04lts-64bit-withps.cfg

	get_raw_results baseline  zipr split split-no-fix-icalls

}

main "$@"



