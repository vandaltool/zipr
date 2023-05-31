#!/bin/bash
set -e
set -x

cd /tmp/peasoup_test
export IDAROOT=$CICD_MODULE_WORK_DIR/idapro82sp1py3
export IDASDK=$CICD_MODULE_WORK_DIR/idapro82_sdk
source set_env_vars

cd $PEASOUP_HOME/tests/rust_test/
./testit.sh
