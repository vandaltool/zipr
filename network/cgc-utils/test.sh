#!/bin/bash

if [ $# -lt 1 ]; then
	echo "usage: <dir> <optional ps_analyze.sh parameters>"
	exit 1
fi

dir=$1
shift

cd /techx_share/techx_umbrella/peasoup/ 1>&2
source set_env_vars 1>&2
cd - 1>&2

cd $dir > /dev/null 2>&1;
if [ $? -ne 0 ]; then
	echo "Invalid test directory"
	exit 1
fi

make clean 1>&2
make DO_ZIPR=1 EXTRA_PS="$*" 1>&2
/home/vagrant/cgc-utils/find_maxrsss.sh minflt
/home/vagrant/cgc-utils/find_maxrsss.sh maxrss
/home/vagrant/cgc-utils/find_maxrsss.sh sw-cpu-clock
/home/vagrant/cgc-utils/find_maxrsss.sh sw-task-clock
make clean 1>&2
rm -rf peasoup_executable* 1>&2
exit 0
