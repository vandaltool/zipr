#!/bin/bash
set -e
set -x

cd /tmp/peasoup_test
export IDAROOT=$CICD_MODULE_WORK_DIR/idapro71
export IDASDK=$CICD_MODULE_WORK_DIR/idapro71_sdk
source set_env_vars

cd $PEASOUP_HOME/tests/go_test/
./testit.sh
