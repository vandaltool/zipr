#!/bin/bash
set -e
set -x

cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
source set_env_vars

cd $PEASOUP_HOME/tests
make clean

# verify "fail" configuration
./test_cmds.sh -l -c fail -a ls
./test_cmds.sh -l -c fail -a grep

exit 0

