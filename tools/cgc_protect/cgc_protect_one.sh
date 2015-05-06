#!/bin/bash

#
# Given a directory full of POVs, protect a singleton CB
# by sandboxing the appropriate instructions 
# 
#

CGC_BIN=$1            # input cgc binary
CGC_PROTECTED=$2      # output(protected) cgc binary
POV_DIR=$3            # directory containing POVs 

benchmark=`basename $CGC_BIN _`
log=tmp.log.$$

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
	sudo cb-test --debug --xml ${one_pov} --timeout 20 --directory ${binary_dir} --cb ${binary} --log $log 
	grep "core identified" $log
	if [ $? -eq 0 ]; then
		if [ -f $core ]; then
			sudo chown `whoami` $core 
			eip=`$PEASOUP_HOME/tools/extract_eip_from_core.sh ${CGC_BIN} $core`
			if [ $? -eq 0 ]; then
				echo "$eip" >> $CRASH_SITES
			fi
			echo "EIP: $eip"
		fi
	fi

	sudo rm $core 2>/dev/null
done

#
# sandbox all uncovered faulting instructions
#
if [ -f $CRASH_SITES ]; then
	tmp=tmp.$$
	sort $CRASH_SITES | uniq > $tmp
	mv $tmp $CRASH_SITES

	cso_file="crashes.cso"
	rm $cso_file 2>/dev/null

	while read -r LINE || [[ -n $LINE ]]; do
	echo "$benchmark,$LINE,,Tainted Dereference" >> $cso_file
	done < $CRASH_SITES

	echo "CRASH SITES UNCOVERED:"
	cat $cso_file

	if [ -f $cso_file ]; then
		$PEASOUP_HOME/tools/ps_analyze_cgc.sh $CGC_BIN $CGC_PROTECTED --step-option watch_allocate:--warning_file=`pwd`/$cso_file
		rm -fr peasoup_executable_directory*
	else
		exit 1
	fi
fi

sudo rm $log 2>/dev/null
rm $CRASH_SITES 2>/dev/null

exit 0
