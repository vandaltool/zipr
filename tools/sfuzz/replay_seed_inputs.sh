#!/bin/bash

cgc_binary=$1            # binary 
seeds_dir=$2             # path of seed input directory
crash_dir=$3             # where to copy any crashing inputs
crash_eip_file=$4        # out: name of file where we output the crashing EIPs

#seeds_dir=${PEASOUP_HOME}/tools/sfuzz/seed_inputs

TIMEOUT=10

# make sure we can get a core file
ulimit -c unlimited

#
# feed seed inputs to binary
#
for i in `ls $seeds_dir`
do
	input=${seeds_dir}/$i
	rm core &> /dev/null
	timeout $TIMEOUT $cgc_binary < $input
	if [ -f core ]; then
		cp $input $crash_dir
		eip=`${PEASOUP_HOME}/tools/extract_eip_from_core.sh $cgc_binary core`
		if [ $? -eq 0 ]; then
			echo $eip >> $crash_eip_file			
		fi

                echo "EIP: $eip"
	fi
done

if [ -f $crash_eip_file ]; then
	sort $crash_eip_file | uniq > tmp.$$
	mv tmp.$$ $crash_eip_file
fi
