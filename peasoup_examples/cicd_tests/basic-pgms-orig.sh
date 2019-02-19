#!/bin/bash
set -e
set -x

cd $CICD_MODULE_WORK_DIR/ps_pe_umbrella
source set_env_vars

# internal tests that do not require transforming binaries
cd $PEASOUP_HOME/tests
make clean; ./test_cmds.sh -c orig -l

