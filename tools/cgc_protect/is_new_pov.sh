#!/bin/bash

#
# Returns success (0) only if the POV results in a new crash point 
#

POV_PATH=$1       # fully qualified path for POV
CRASH_SUMMARY=$2  # input/output: POV-->crash summary file
CGC_BIN=$3        # input cgc binary (@todo: handle multi-cbs)

cbtest=$CGC_UMBRELLA_DIR/scripts/techx-cb-test
timeout=20
delimiter="###"

log=`pwd`/tmp.log.$$

pov_base=`basename ${POV_PATH}`
binary=`basename $CGC_BIN`
binary_dir=`dirname $CGC_BIN`
core=${binary_dir}/core

# lookup pov
tmp=`grep -F "${pov_base}${delimiter}" ${CRASH_SUMMARY}`
if [ $? -eq 0 ];then
	return 1
fi

# cleanup any stale core files
if [ -f $core ]; then
	sudo rm $core 2>/dev/null
fi

echo "sudo -E $cbtest --debug --xml ${pov_base} --timeout $timeout --directory ${binary_dir} --cb ${binary} --log $log"
sudo -E $cbtest --debug --xml ${pov_base} --timeout $timeout --directory ${binary_dir} --cb ${binary} --log $log 
grep "core identified" $log
if [ $? -eq 0 ]; then
	if [ -f $core ]; then
		echo "is_new_pov.sh: core file found"
		sudo chown `whoami` $core 
		eip=`timeout $timeout $PEASOUP_HOME/tools/extract_eip_from_core.sh ${CGC_BIN} $core`
		if [ $? -eq 0 ]; then
			echo "$eip" >> $CRASH_SITES
			return 0
		else
			return 1
		fi

		sudo rm $core 2>/dev/null
	else
		return 1
	fi
else
	return 1
fi
