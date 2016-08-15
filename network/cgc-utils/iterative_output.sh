#!/bin/bash

output_dir=$1
shift
process="$@"
i=`ls -lAt $output_dir | grep -v 'total' | head -n 1 | awk '{print $9;}'`
$process > /${output_dir}/$$.$i
diff ${output_dir}/$i /${output_dir}/$$.$i > /dev/null
if [ $? -ne 0 ]; then
	echo "Saw a difference!"
	cp /${output_dir}/$$.$i ${output_dir}/$((i+1))
fi
rm -f /${output_dir}/$$.$i
