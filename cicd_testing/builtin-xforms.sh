#!/bin/bash
set -e
set -x

cd /tmp/peasoup_test
export IDAROOT=$CICD_MODULE_WORK_DIR/idapro71
export IDASDK=$CICD_MODULE_WORK_DIR/idapro71_sdk
source set_env_vars
cd $PEASOUP_HOME/tests

make clean; 
if [[ $(uname -m) == 'armv7l' ]] || [[ $(uname -m) == 'aarch64' ]]; then
	./test_cmds.sh -c "rida" -l -a "du ls"
else
	./test_cmds.sh -c "rida fix_calls_rida fix_calls_ida" -l -a "bzip2 tcpdump"
	if lsb_release -d | grep 'Ubuntu 20.04.4 LTS'  ; then
		cd $PEASOUP_HOME/test/empty
		./testit.sh
	fi
fi



