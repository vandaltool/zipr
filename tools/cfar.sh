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
config_name="unspecified"
backend="strata"

cmd_line_options=( "$@" )
declare -a new_cmd_line_options

max="${#cmd_line_options[@]}"
seq=0

#
# Parse out options for peasoup and learn some things, err on some things that cfar isn't (yet) supporting.
#
while [ $seq -lt $max ]; 
do
	i=${cmd_line_options[$seq]}
	# this option is for cfar, handle it and remove it from the ps_analyze arguments.
	if [ "$i" == "--structured_p1_canaries" ]; then 	
		structured_p1_canaries=1
	# this option is for cfar, handle it and remove it from the ps_analyze arguments.
	elif [ "$i" == "--structured_noc" ]; then 	
		structured_noc=1
	# this option is for cfar, handle it and remove it from the ps_analyze arguments.
	elif [ "$i" == "--config_name" ]; then 	
		seq=$(expr $seq + 1)
		config_name=${cmd_line_options[$seq]}
		echo "Found config_name setting, config=$config_name"
	# this option is used by cfar, and the user cannot request it.
	elif [ "$i" == "--tempdir" ]; then 	
		echo "cfar.sh cannot accept --tempdir as it uses it internally."
		exit 1
	# monitor the backend specified by the user, and remember it, but do pass ita long.
	elif [ "$i" == "--backend" ]; then 	
		# include this in the ps_analyze options.
		new_cmd_line_options+=("$i")
		if [ ${cmd_line_options[$(expr $seq + 1)]} = "zipr" ];  then
			backend="zipr"
		elif [ ${cmd_line_options[$(expr $seq + 1)]} = "strata" ];  then
			backend="strata"
		else
			echo "Unknown backend: ${cmd_line_options[$(expr $seq + 1)]}"
			exit 1
		fi
	else
		new_cmd_line_options+=("$i")
	fi

	seq=$(expr $seq + 1)
done


# add default options for cfar which asks ps_analyze to fill in a variant specification for what it did.
new_cmd_line_options+=(--step generate_variant_config=on)

#
# figure out a place for ps_analyze to work so we can examine results.
#
outbase=$(basename $out)
baseoutdir=$(dirname $out)/peasoup_executable_dir.$outbase.$config_name.$$

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
	declare -a per_variant_options
	per_variant_options=()
	if [ $structured_noc  -eq 1 ]; then
		# the path to the "shared memory" that cfar is using.
		sharepath_key="$seq:$variants:dir://$share_path"
		per_variant_options+=(--step-option zipr:"--large_only:variant $sharepath_key")
	fi
	
	# options to p1 to create non-overlapping canary values.
	if [ $structured_p1_canaries  -eq 1 ]; then
		per_variant_options+=(--step-option p1transform:"--canary_value 0xFF0${seq}${seq}0FF --random_seed $anyseed")
	fi

	# add in options for output directory.
	per_variant_options+=(--tempdir "$baseoutdir.v${seq}")

	# invoke $PS.
	echo PGDATABASE=peasoup_${USER}_v$seq $zipr_env $PEASOUP_HOME/tools/ps_analyze.sh $in $out.v$seq "${new_cmd_line_options[@]}"  "${per_variant_options[@]}" 
	PGDATABASE=peasoup_${USER}_v$seq $zipr_env $PEASOUP_HOME/tools/ps_analyze.sh $in $out.v$seq "${new_cmd_line_options[@]}"  "${per_variant_options[@]}" > variant_output.$seq 2>&1 &

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
		echo "Protection process $i failed with excode code: $exit_code."
		ok=0
	fi
done


# report success/failures.
if [ $ok != 1 ] ; then
	echo
	echo
	echo "Some variants failed"
	echo
	echo
	exit 1
else
	echo "Successfully protected $variants variants, attempting to generate MVEE configuration files"
fi

$PEASOUP_HOME/tools/generate_mvee_config.sh  "$variants" "$out" "$baseoutdir" "$backend"
