#!/bin/bash
set -e
set -x

cd $CICD_MODULE_WORK_DIR/peasoup_test
source set_env_vars

cd $SECURITY_TRANSFORMS_HOME/libIRDB-elfdep/test/
./test-elfdep.sh
