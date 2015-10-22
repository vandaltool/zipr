#!/bin/sh

variants=$1
in=$2
out=$3
shift
shift
shift

share_path=/tmp
pids=

rm -f $share_path/Barriers*

anyseed=$$

for seq in $(seq 0 $(expr $variants - 1) )
do

	sharepath_key="$seq:$variants:dir://$share_path "
	zipr_env="ZIPR_VARID=$sharepath_key"
	p1options=" --step-option p1transform:--canary_value --step-option p1transform:0xFF0${seq}${seq}0FF --step-option p1transform:--random_seed --step-option p1transform:$anyseed "
	cmd=" PG_DATABASE=peasoup_`whoami`_$seq $zipr_env $PEASOUP_HOME/tools/ps_analyze.sh $in $out.v$seq $@ $p1options > peasoup.v$seq 2>&1 &"
	echo $cmd
	eval $cmd
	pids="$pids $!"

done

ok=1

for i in $pids;
do
	wait $i
	exit_code=$?
	if [ $exit_code != 0 ]; then
		echo "Peasoup process $i failed with excode code: $exit_code."
		ok=0
	fi
done

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

