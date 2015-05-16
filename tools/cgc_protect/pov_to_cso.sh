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

log=tmp.log.$$

cbtest=$CGC_UMBRELLA_DIR/scripts/techx-cb-test

CRASH_SITES=tmp.crashes.$$

# run cb-test on each POV invidually
for i in `ls ${POV_DIR}/*.xml`
do
	echo ""
	echo $i

	one_pov=$i
	binary=`basename $CGC_BIN`
	binary_dir=`dirname $CGC_BIN`
	core=${binary_dir}/core

	sudo rm $core 2>/dev/null
	sudo $cbtest --debug --xml ${one_pov} --timeout 20 --directory ${binary_dir} --cb ${binary} --log $log 
	grep "core identified" $log
	if [ $? -eq 0 ]; then
		if [ -f $core ]; then
			sudo chown `whoami` $core 
			eip=`timeout 20 $PEASOUP_HOME/tools/extract_eip_from_core.sh ${CGC_BIN} $core`
			if [ $? -eq 0 ]; then
				echo "$eip" >> $CRASH_SITES
			fi
			echo "EIP: $eip"
		fi
	fi

	sudo rm $core 2>/dev/null
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

sudo rm $log 2>/dev/null
rm $CRASH_SITES 2>/dev/null

exit 0
