#!/bin/bash

source $(dirname $0)/ps_wrapper.source $0

is_so()
{
	file $1 | egrep "LSB *shared object" > /dev/null

	if [ $? = 0 ]; then
		echo 1
	else
		echo 0
	fi
}


# parse first 3 parameters as fixed position params.
variants=$1
in=$2
in_base=`basename $in`
out=$3
shift
shift
shift


structured_p1_canaries=0
structured_stack_stamp=0
structured_noc=0
structured_nog=0
structured_nos=0
structured_ds=0
structured_stack_init=0   # auto stack initialize
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
	elif [ "$i" == "--structured_stack_stamp" ]; then 	
		structured_stack_stamp=1
	# this option is for cfar, handle it and remove it from the ps_analyze arguments.
	elif [ "$i" == "--structured_noc" ]; then 	
		structured_noc=1
	# this option is for cfar, handle it and remove it from the ps_analyze arguments.
	elif [ "$i" == "--structured_nog" ]; then 	
		structured_nog=1
	# this option is for cfar, handle it and remove it from the ps_analyze arguments.
	elif [ "$i" == "--structured_nos" ]; then 	
		structured_nos=1
	# this option is for cfar, handle it and remove it from the ps_analyze arguments.
	elif [ "$i" == "--structured_ds" ]; then 	
		structured_ds=1
	elif [ "$i" == "--structured_stack_init" ]; then 	
		structured_stack_init=1
	# this option is for cfar, handle it and remove it from the ps_analyze arguments.
	elif [ "$i" == "--config_name" ]; then 	
		seq=$(expr $seq + 1)
		config_name=${cmd_line_options[$seq]}
		echo -n "	"
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
# and also by default dump the IRDB mapping information, useful for debugging.
new_cmd_line_options+=(--step generate_variant_config=on --step dump_map=on --step-option zipr:"--add-sections true --bss-opts false")

#
# figure out a place for ps_analyze to work so we can examine results.
#
outbase=$(basename $out)

if  [ $(is_so $in) = 0 ]; then
	baseoutdir=${out}/target_apps/dh-${in_base}/${config_name}
else 
	baseoutdir=${out}/target_app_libs/dh-${in_base}/${config_name}
fi

if [ -d $baseoutdir ]; then
	echo "Directory $baseoutdir already exists."
	echo "Skipping duplicate work."
	exit 0
fi


# init some variables.
share_path=/tmp/$(whoami)

declare -A pidSet

# remove any old data xfers.
rm -f $share_path/Barriers*


# pick a random seed for this run.
anyseed=$$

# create a copy of ps_analyze for each variant we want to create.
for seq in $(seq 0 $(expr $variants - 1) )
do
	# setup an array to pass hold variant-specific options. 
	declare -a per_variant_options
	per_variant_options=()

	# options for zipr's large_only plugin to help create non-overlapping code segments. 
	if [ $structured_noc  -eq 1  -o  $structured_nog -eq 1 ]; then
		# the path to the "shared memory" that cfar is using.
		sharepath_key="$seq:$variants:dir://$share_path"
		per_variant_options+=(--step-option zipr:"--large_only:variant $sharepath_key")
	fi

	if [ $structured_nos  -eq 1 ]; then
		sharepath_key="$seq:$variants:dir://$share_path"
		per_variant_options+=(--step-option non_overlapping_stack:"--mode structured --barrier $sharepath_key")
	fi
	
	# options to p1 to create non-overlapping canary values.
	if [ $structured_p1_canaries  -eq 1 ]; then
		per_variant_options+=(--step-option p1transform:"--canary_value 0x100${seq}${seq}000 --random_seed $anyseed")
	fi

	if [ $structured_ds -eq 1 ]; then
		sharepath_key="$seq:$variants:dir://$share_path"
		per_variant_options+=(--step-option duck_season:"--barrier $sharepath_key")
	fi

	if [ $structured_stack_init -eq 1 ]; then
		# check even/odd status of variant number.
		if [ $(expr ${seq} % 2) = 0 ]; then
			per_variant_options+=(--step-option initialize_stack:"--initvalue 0x00000000")
		else
			per_variant_options+=(--step-option initialize_stack:"--initvalue 0xffffffff")
		fi
	fi

	# options to stack_stamp to create non-overlapping stamps
	if [ $structured_stack_stamp  -eq 1 ]; then
		# check even/odd status of variant number.
		if [ $(expr ${seq} % 2) = 0 ]; then
			# even variants get a5 * 4.  this is 01010101... in binary.
			per_variant_options+=(--step-option stack_stamp:"--stamp-value 0xa5a5a5a5")
		else
			# even variants get 5a * 4.  this is 10101010... in binary.
			per_variant_options+=(--step-option stack_stamp:"--stamp-value 0x5a5a5a5a")
		fi
	fi

	# add in options for output directory.
	per_variant_options+=(--tempdir "$baseoutdir/v${seq}/peasoup_executable_dir")
	mkdir -p "$baseoutdir/v${seq}"

	myin=$(echo $in|sed "s/<<VARNUM>>/$seq/g")

	# invoke $PS.
	#echo "PGDATABASE=peasoup_${USER}_v$seq $zipr_env $PS $in $baseoutdir/v${seq}/${in_base} " "${new_cmd_line_options[@]}"  "${per_variant_options[@]}" 
	echo -n "	"
	(set -x; env PGDATABASE=peasoup_${USER}_v$seq $zipr_env $PS $myin $baseoutdir/v${seq}/${in_base} "${new_cmd_line_options[@]}"  "${per_variant_options[@]}" > $baseoutdir/v${seq}/variant_output.txt 2>&1 ) & 

	# remember the pid.
	pidSet["$!"]=1

done


# mark that no one has detected a failure yet.
ok=1


#
# wait for each child.  detect failures.
#

# while there are children we haven't queried their exit status
while [[ ${#pidSet[@]} != 0 ]] ; 
do
	# sleep so this loop doesn't consume too many cycles.
	sleep 2

	# check each un-checked child to see if it's exited
	for child in "${!pidSet[@]}"; 
	do 


		kill -0 $child > /dev/null 2>&1
		dead=$?
		# if it's dead, mark it as so, and get the exit code
		if [[ $dead = 1 ]]; then
			# mark as dead
			unset pidSet["$child"]

			# get exit code
			wait $child
			exit_code=$?

			# sanity check exit code
			if [ $exit_code == 0 ]; then
				echo "	Protection process for $child went well!  Exit_code: $exit_code."
			elif [ $exit_code == 1 ]; then
				echo "	Protection process for $child had warnings.  Exit_code: $exit_code."
			else
				echo "******************************************************************"
				echo "*Protection process for $child failed with exit code: $exit_code.*"
				echo "******************************************************************"

				# on an error, kill all remaining jobs.
				local count=0
				while [[ $(jobs -p) != "" ]] && [[ $count -lt 10 ]] ;
				do
					echo "Killing remaining jobs."
					kill -9 $(jobs -p)
					sleep 1
					count=$(expr $count + 1 )
				done

				# mark that we don't have to check other children.
				pidSet=()

				# mark that there's a failure observed
				ok=0
	
				# quit checking
				break
			fi
		fi
		
	done
done


# report success/failures.
if [ $ok != 1 ] ; then
	echo
	echo
	echo "Some variants failed"
	echo
	echo
	exit 2
else
	echo "Successfully protected $variants variants" 
fi

