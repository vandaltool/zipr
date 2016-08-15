#!/bin/bash

# Collapse multiple rows per CB into a single row - makes
# the results easier to graph in Excel, etc.

if [ $# -ne 1 ]; then
	echo "$0 <filename>"
	exit
fi

input_filename=$1
temp_filename1=${input_filename}.1
temp_filename2=${input_filename}.2
output_filename=${input_filename}.collapsed

cat ${input_filename} | sed s/sum://g | sed s/avg://g | sed s/n://g > ${temp_filename1}
cat ${temp_filename1} | tr '\n' '#' | awk --field-separator=# '{ for (i=1;i<NF;i++) { printf $i; if (!(i % 2)) print ""; } }' > ${temp_filename2}

cp ${temp_filename2} ${output_filename}

rm -f ${temp_filename1}
rm -f ${temp_filename2}
