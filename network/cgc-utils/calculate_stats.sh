#!/bin/bash

d=`pwd`
output_filename="/tmp/cgc_stats.output"
echo "" > $output_filename
for i in `ls`; do
	echo $i
	if [ ! -d "$i" ]; then
		continue
	fi
	cd $i;
	make clean
	make DO_ZIPR=1
	/usr/share/cb-testing/find_maxrsss.sh minflt >> $output_filename
	/usr/share/cb-testing/find_maxrsss.sh maxrss >> $output_filename
	/usr/share/cb-testing/find_maxrsss.sh sw-cpu-clock >> $output_filename
	/usr/share/cb-testing/find_maxrsss.sh sw-task-clock >> $output_filename
	make clean
	rm -rf peasoup_executable*
	cd $d
	/home/vagrant/postgres_setup.sh
done
