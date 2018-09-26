#!/bin/bash 

start=0
end=10000
orig=$(realpath `which du`) 

protect()
{
	stop_at=$1
	out=$2

	echo "Trying first $stop_at xforms."
	set -x

	ZAFL_LIMIT_END=$stop_at zafl.sh $orig $out -o zafl:-v --tempdir results/peasoup.bin_search.$stop_at  > $out.ps_output 2>&1

	set +x

}

get_test_result()
{
	curdir=$PWD
	exe=$(realpath $1)
	touch $2
	exe_output=$(realpath $2)

	$exe -bh /home/an7s/zafl_umbrella/zipr_umbrella/peasoup_examples/tests/du/data >$exe_output 2>&1
	echo $? >> $exe_output
}


####################################################################################################
# should need no changes after here
####################################################################################################

bin_search()
{
	my_start=$1
	my_end=$2 
	my_orig=$3
	my_out_pattern=$4
	my_exe_pattern=$5
	my_correct_results=$6

	while [[ $my_start -lt $(($my_end - 1)) ]] 
	do
		mid=$(( ($my_start + $my_end) / 2 ))
		echo "start=$my_start end=$my_end mid=$mid"
		protect $mid $exe_pattern.$mid
		get_test_result $exe_pattern.$mid $out_pattern.$mid 
		cmp $my_correct_results $my_out_pattern.$mid > /dev/null 2>&1 
		if [ $? != 0 ];  then
			# diff observed 
			echo "Detected that $mid generates differences";
			my_end=$(($mid)) 
		else
			echo "Detected that $mid generates the same output";
			my_start=$(($mid))
		fi

	done
	echo "Binary Search Complete. correct=$my_start broken=$my_end"

}

main()
{
	out_pattern=results/bin_search.out
	exe_pattern=results/$(basename $orig).xform
	correct_results=correct.txt


	get_test_result $orig $correct_results
	protect $start $exe_pattern.$start
	get_test_result $exe_pattern.$start $out_pattern.$start 
	cmp correct.txt $out_pattern.$start > /dev/null 2>&1 
	if [ $? != 0 ];  then
		echo "starting point also fails test."
		exit 1
	fi

	protect $end $exe_pattern.$end
	get_test_result $exe_pattern.$end $out_pattern.$end 
	cmp correct.txt $out_pattern.$end > /dev/null 2>&1 
	if [ $? == 0 ];  then
		echo "Ending point doesn't fail test!"
		exit 1
	fi

	bin_search $start $end $orig $out_pattern $exe_pattern $correct_results

	exit 0
	
}

main




