#!/bin/bash

#
# Given a directory full of POVs, protect a singleton CB
# by sandboxing the appropriate instructions 
#

CGC_BIN=$1       # input cgc binary
CGC_CSID=$2      # cgc name
POV_DIR=$3       # directory containing POVs 
CSO_FILE=$4      # output: CSO warning file suitable for sandboxing step
POV_CRASH_SUMMARY_FILE=$5   # input/output: POV/raw inputs-->crash summary file
CRASH_DIR=$6     # directory with raw crashing inputs

timeout=20
local_crash_summary=tmp.crash.summary.$$
log=`pwd`/tmp.log.$$

cbtest=$CGC_UMBRELLA_DIR/scripts/techx-cb-test

delimiter="###"

CRASH_SITES=tmp.crashes.$$

ulimit -c unlimited

# copy the crash summary file locally
cp $POV_CRASH_SUMMARY_FILE $local_crash_summary

# run cb-test on each POV invidually
if [ -d ${POV_DIR} ]; then

for i in `ls ${POV_DIR}/*.xml`
do
	echo ""
	echo $i

	one_pov=$i
	pov_base=`basename $one_pov`

	binary=`basename $CGC_BIN`
	binary_dir=`dirname $CGC_BIN`
	core=${binary_dir}/core

	# lookup pov
	tmp=`grep -F "${pov_base}" $local_crash_summary`
	if [ $? -eq 0 ];then
		eip=`echo $tmp | awk -F"${delimiter}" '{print $2}'`
		echo $eip | grep "0x0"
		if [ ! $? -eq 0 ]; then
			echo "$eip" >> $CRASH_SITES
		fi
		echo "Found pov: ${pov_base} in cache -- eip = $eip"
		continue
    else
		echo "POV ${pov_base} not found in cache -- attempt to extract crashing instruction"
	fi

	# cleanup any stale core files
	if [ -f $core ]; then
		sudo rm $core 2>/dev/null
	fi

	echo "sudo -E $cbtest --debug --xml ${one_pov} --timeout $timeout --directory ${binary_dir} --cb ${binary} --log $log"
	sudo -E $cbtest --debug --xml ${one_pov} --timeout $timeout --directory ${binary_dir} --cb ${binary} --log $log 
	grep "core identified" $log
	if [ $? -eq 0 ]; then
		if [ -f $core ]; then
                        echo "pov_to_cso.sh: core file found"
			sudo chown `whoami` $core 
			eip=`timeout $timeout $PEASOUP_HOME/tools/extract_eip_from_core.sh ${CGC_BIN} $core`
			if [ $? -eq 0 ]; then
				echo "$eip" >> $CRASH_SITES
				echo "${pov_base}${delimiter}${eip}" >> $local_crash_summary
			else
				echo "${pov_base}${delimiter}0x0" >> $local_crash_summary
			fi

			sudo rm $core 2>/dev/null
		else
                        echo "pov_to_cso.sh: cannot find core file"
			echo "${pov_base}${delimiter}0x0" >> $local_crash_summary
		fi
	else
		echo "${pov_base}${delimiter}0x0" >> $local_crash_summary
	fi
done

fi

#
# Extract crash sites from crashing input dir (if any)
#
if [ -d $CRASH_DIR ]; then
	echo "crash directory was specified: $CRASH_DIR"
	for i in `ls ${CRASH_DIR}/*`
	do
		# lookup crash input
		crash_base=`basename ${i}`
		tmp=`grep -F "${crash_base}${delimiter}" $local_crash_summary`
		if [ $? -eq 0 ];then
			eip=`echo $tmp | awk -F"${delimiter}" '{print $2}'`
			echo $eip | grep "0x0"
			if [ ! $? -eq 0 ]; then
				echo "$eip" >> $CRASH_SITES
			fi
			echo "Found crash: ${crash_base} in cache -- eip = $eip"
			continue
		else
			echo "crashing input ${crash_base} not found in cache -- attempt to extract crashing instruction"
		fi
               
		eip=`timeout $timeout ${PEASOUP_HOME}/tools/replay_with_gdb.sh ${CGC_BIN} ${i}`
		if [ $? -eq 0 ]; then
			# segmentation fault detected and valid eip
			echo "detected valid crash site: $eip"
			echo $eip >> $CRASH_SITES
			echo "${crash_base}${delimiter}${eip}" >> $local_crash_summary
		else
			echo "no valid crash site detected: $eip"
			echo "${crash_base}${delimiter}0x0" >> $local_crash_summary
		fi
	done
fi

# local_crash_summary should have a list of all potential crash sites
# extract all the instructions to sandbox
grep "${delimiter}" $local_crash_summary | awk -F"${delimiter}" '{print $2}' | sort | uniq >> $CRASH_SITES
			
#
# generate policy file for input to sandboxing step
#

if [ -f $CRASH_SITES ]; then
	tmp=tmp.$$
	grep -v "0x0\$" $CRASH_SITES | sort | uniq > $tmp
	mv $tmp $CRASH_SITES

	while read -r LINE || [[ -n $LINE ]]; do
		echo "$CGC_CSID,$LINE,,Tainted Dereference" >> $CSO_FILE
	done < $CRASH_SITES
fi

# mv crash summary file out
sort $local_crash_summary | uniq > tmp.$$
mv tmp.$$ ${POV_CRASH_SUMMARY_FILE}

sudo rm $log 2>/dev/null
rm $CRASH_SITES 2>/dev/null
killall `basename $CGC_BIN`

exit 0
