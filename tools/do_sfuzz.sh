#!/bin/bash -x

# pre: we're in the peasoup subdir

binary=$1
benchmark=$2
cso_file=$3

mkdir -p sfuzz/crashes

seeds_dir=${PEASOUP_HOME}/tools/sfuzz/seed_inputs
crash_dir=sfuzz/crashes
crash_eip_file=sfuzz/crashing_eips 

${PEASOUP_HOME}/tools/sfuzz/replay_seed_inputs.sh ./$binary $seeds_dir $crash_dir $crash_eip_file

echo "Found the following crasheing EIPs:"
cat $crash_eip_file

# need file to be in format
#benchmark,address,bufsize,type

while read -r LINE || [[ -n $LINE ]]; do
echo "$benchmark,$LINE,,Tainted Dereference" >> $cso_file
done < $crash_eip_file





exit 0
