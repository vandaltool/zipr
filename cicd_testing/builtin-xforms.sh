#!/bin/bash
set -e
set -x

cd /tmp/peasoup_test
source set_env_vars
cd $PEASOUP_HOME/tests

make clean; 
if [[ $(uname -m) == 'armv7l' ]] || [[ $(uname -m) == 'aarch64' ]]; then
	./test_cmds.sh -c "zipr_rida" -l -a "bzip2 ls"
else
	./test_cmds.sh -c "zipr_rida fix_calls_rida fix_calls_ida" -l -a "bzip2 tcpdump"
fi

