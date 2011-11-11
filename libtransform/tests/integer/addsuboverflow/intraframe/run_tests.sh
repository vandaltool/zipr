#!/bin/sh
outfile=tests.out

rm $outfile
touch $outfile

for i in `ls addsub*.sh`
do
	echo running $i
	$TEST_HARNESS_HOME/run_one_test.sh $i no_redirect >> $outfile
	
	if [ ! $? -eq 0 ]; then
		echo Test $i failed >> $outfile
	else
		echo Test $i success >> $outfile
	fi
done
