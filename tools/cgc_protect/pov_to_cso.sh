#!/bin/bash

#
# Given a directory full of POVs, protect a singleton CB
# by sandboxing the appropriate instructions 
# 
#

CGC_BIN=$1       # input cgc binary
CGC_CSID=$2      # cgc name
POV_DIR=$3       # directory containing POVs 
CSO_FILE=$4      # output: CSO warning file suitable for sandboxing step
POV_CRASH_SUMMARY_FILE=$5   # input/output: POV-->crash summary file

local_crash_summary=tmp.crash.summary.$$
log=tmp.log.$$

cbtest=$CGC_UMBRELLA_DIR/scripts/techx-cb-test

delimiter="###"

CRASH_SITES=tmp.crashes.$$

# copy the crash summary file locally
cp $POV_CRASH_SUMMARY_FILE $local_crash_summary

# run cb-test on each POV invidually
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

	echo "sudo $cbtest --debug --xml ${one_pov} --timeout 20 --directory ${binary_dir} --cb ${binary} --log $log"
	sudo $cbtest --debug --xml ${one_pov} --timeout 20 --directory ${binary_dir} --cb ${binary} --log $log 
	grep "core identified" $log
	if [ $? -eq 0 ]; then
		if [ -f $core ]; then
			sudo chown `whoami` $core 
			eip=`timeout 20 $PEASOUP_HOME/tools/extract_eip_from_core.sh ${CGC_BIN} $core`
			if [ $? -eq 0 ]; then
				echo "$eip" >> $CRASH_SITES
				echo "${pov_base}${delimiter}${eip}" >> $local_crash_summary
			else
				echo "${pov_base}${delimiter}0x0" >> $local_crash_summary
			fi

			sudo rm $core 2>/dev/null
		else
			echo "${pov_base}${delimiter}0x0" >> $local_crash_summary
		fi
	else
		echo "${pov_base}${delimiter}0x0" >> $local_crash_summary
	fi


done

#
# generate policy file for input to sandboxing step
#
if [ -f $CRASH_SITES ]; then
	tmp=tmp.$$
	sort $CRASH_SITES | uniq > $tmp
	mv $tmp $CRASH_SITES

	while read -r LINE || [[ -n $LINE ]]; do
		echo "$CGC_CSID,$LINE,,Tainted Dereference" >> $CSO_FILE
	done < $CRASH_SITES
fi

# mv crash summary file out
mv $local_crash_summary ${POV_CRASH_SUMMARY_FILE}

sudo rm $log 2>/dev/null
rm $CRASH_SITES 2>/dev/null

exit 0
