#!/bin/sh


# parse first 3 parameters as fixed position params.
variants=$1
in=$2
out=$3
shift
shift
shift


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

	# the path to the "shared memory" that cfar is using.
	sharepath_key="$seq:$variants:dir://$share_path "

	# optoins for zipr's large_only plugin to help create non-overlapping code segments. 
	large_only_options="--step-option zipr:--large_only:on --step-option zipr:true --step-option zipr:--large_only:variant --step-option zipr:$sharepath_key"
	
	# optoins to p1 to create non-overlapping canary values.
	p1options=" --step-option p1transform:--canary_value --step-option p1transform:0xFF0${seq}${seq}0FF --step-option p1transform:--random_seed --step-option p1transform:$anyseed "

	# invoke $PS.
	cmd=" PGDATABASE=peasoup_${USER}_v$seq $zipr_env $PEASOUP_HOME/tools/ps_analyze.sh $in $out.v$seq $@ $p1options $large_only_options > variant_output.$seq 2>&1 &"
	echo $cmd
	eval $cmd

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
