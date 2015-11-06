#!/bin/bash


# parse first 3 parameters as fixed position params.
variants=$1
in=$2
out=$3
shift
shift
shift


structured_p1_canaries=0
structured_noc=0

cmd_line_options=( "$@" )
declare -a new_cmd_line_options
for i in "${cmd_line_options[@]}"
do
	if [ "$i" == "--structured_p1_canaries" ]; then 	
		echo "Found structured p1 canaries option"
		structured_p1_canaries=1
	elif [ "$i" == "--structured_noc" ]; then 	
		echo "Found structured non-overlapping code option "
		structured_noc=1
	else
		new_cmd_line_options+=("$i")
	fi
done


# init some variables.
share_path=/tmp
pids=


# remove any old data xfers.
rm -f $share_path/Barriers*


# pick a random seed for this run.
anyseed=$$

# create a copy of ps_analyze for each variant we want to create.
for seq in $(seq 0 $(expr $variants - 1) )
do
	# optoins for zipr's large_only plugin to help create non-overlapping code segments. 
	declare -a large_only_options
	if [ $structured_noc  -eq 1 ]; then
		# the path to the "shared memory" that cfar is using.
		sharepath_key="$seq:$variants:dir://$share_path"
		large_only_options=(--step-option zipr:"--large_only:variant $sharepath_key")
	fi
	
	# options to p1 to create non-overlapping canary values.
	declare -a p1options
	if [ $structured_p1_canaries  -eq 1 ]; then
		p1options=(--step-option p1transform:"--canary_value 0xFF0${seq}${seq}0FF --random_seed $anyseed")
	fi

	# invoke $PS.
	#cmd=env PGDATABASE=peasoup_${USER}_v$seq $zipr_env $PEASOUP_HOME/tools/ps_analyze.sh $in $out.v$seq ${cmd_line_options[@]}  ${p1options[@]} ${large_only_options[@]} "
	#echo "$cmd"
	#eval $cmd > variant_output.$seq 2>&1 &

	echo PGDATABASE=peasoup_${USER}_v$seq $zipr_env $PEASOUP_HOME/tools/ps_analyze.sh $in $out.v$seq "${new_cmd_line_options[@]}"  "${p1options[@]}" "${large_only_options[@]}" 
	PGDATABASE=peasoup_${USER}_v$seq $zipr_env $PEASOUP_HOME/tools/ps_analyze.sh $in $out.v$seq "${new_cmd_line_options[@]}"  "${p1options[@]}" "${large_only_options[@]}" > variant_output.$seq 2>&1 &

	# remember the pid.
	pids="$pids $!"

done


# mark that no one has detected a failure yet.
ok=1


# wait for each child.  detect failures.
for i in $pids;
do
	wait $i
	exit_code=$?
	if [ $exit_code != 0 ]; then
		echo "Peasoup process $i failed with excode code: $exit_code."
		ok=0
	fi
done


# report success/failures.
if [ $ok = 1 ] ; then
	echo "Successfully generated $variants variants"
	exit 0
else
	echo
	echo
	echo "Some variants failed"
	echo
	echo
	exit 1
fi
