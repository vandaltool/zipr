#!/bin/bash

#
# This script is invoked by crash_filter.py to determine whether a 
# POV results in a new crashing instruction
# 
# Returns success (0) only if the POV results in a new crash point 
# Returns > 0 otherwise
#
# /techx_share/techx_umbrella/peasoup/security_transforms/tools/cgc_protect/is_new_pov.sh /home/vagrant/techx_work/0b32aa01_crash_filter/id:000049,sig:11,src:000007,op:arith8,pos:88,val:+9.10288.xml /home/vagrant/techx_work/0b32aa01_crash_filter/0b32aa01_01.crash.summary /home/vagrant/techx_work/0b32aa01_crash_filter/0b32aa01_01
#

POV_PATH=$1       # fully qualified path for POV
CRASH_SUMMARY=$2  # crash summary file
CGC_BIN=$3        # input cgc binary (@todo: handle multi-cbs)

cbtest=${CGC_UMBRELLA_DIR}/scripts/techx-cb-test
timeout=20
delimiter="###"

log=`pwd`/tmp.log

pov_base=`basename ${POV_PATH}`
binary=`basename $CGC_BIN`
binary_dir=`dirname $CGC_BIN`
core=${binary_dir}/core

# if the pov is already in the crash summary file, then we've seen it before
tmp=`grep -F "${pov_base}${delimiter}" ${CRASH_SUMMARY}`
if [ $? -eq 0 ]; then
	exit 2
fi

# cleanup any stale core files
if [ -f $core ]; then
	sudo rm $core &>/dev/null
fi

# invoke techx-cb-test in an attempt to get a core file
# @todo: input.py has already run the input... at some point we should just
#        have input.py record the crashing instruction so that we don't have
#        to re-run the pov here
echo "sudo -E $cbtest --debug --xml ${POV_PATH} --timeout $timeout --directory ${binary_dir} --cb ${binary} --log $log"
sudo -E $cbtest --debug --xml ${POV_PATH} --timeout $timeout --directory ${binary_dir} --cb ${binary} --log $log 
grep "core identified" $log
if [ $? -eq 0 ]; then
	if [ -f $core ]; then
		echo "is_new_pov.sh: core file found"
		sudo chown `whoami` $core 
		eip=`timeout $timeout $PEASOUP_HOME/tools/extract_eip_from_core.sh ${CGC_BIN} $core`
		if [ $? -eq 0 ]; then
			tmp=`grep -F "${delimiter}$eip" ${CRASH_SUMMARY}`
			if [ $? -eq 0 ]; then
				exit 1
			else
				# new crash instruction, add to summary file
				echo "${pov_base}${delimiter}${eip}" >> ${CRASH_SUMMARY}
				exit 0
			fi
		else
			exit 1
		fi

		sudo rm $core &>/dev/null
	else
		exit 1
	fi
else
	exit 1
fi
