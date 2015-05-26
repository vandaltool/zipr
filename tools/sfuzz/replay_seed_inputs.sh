#!/bin/bash

cgc_binary=$1            # binary 
seeds_dir=$2             # path of seed input directory
crash_dir=$3             # where to copy any crashing inputs
crash_eip_file=$4        # out: name of file where we output the crashing EIPs

TIMEOUT=20

# make sure we can get a core file
ulimit -c unlimited

#
# feed seed inputs to binary
#
for i in `ls $seeds_dir`
do
	input=${seeds_dir}/$i

	eip=`timeout $TIMEOUT ${PEASOUP_HOME}/tools/replay_with_gdb.sh ${cgc_binary} ${input}`
	if [ $? -eq 0 ]; then
		cp $input $crash_dir
        	# segmentation fault detected and valid eip
		echo "detected valid crash site: $eip"
		echo $eip >> $crash_eip_file			
	else
		echo "no valid crash site detected: $eip"
	fi

	echo "EIP: $eip"
done

if [ -f $crash_eip_file ]; then
	sort $crash_eip_file | uniq > tmp.$$
	mv tmp.$$ $crash_eip_file
fi
