#!/bin/bash
set -e
set -x

cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
source set_env_vars

cd $PEASOUP_HOME/tests
make clean

# these tests must fail

# verify ls failure
./test_cmds.sh -l -c fail -a ls
if [ $? -eq 0 ]; then
	exit 1
else
	exit 0
fi

# verify grep failure
./test_cmds.sh -l -c fail -a grep
if [ $? -eq 0 ]; then
	exit 1
else
	exit 0
fi

