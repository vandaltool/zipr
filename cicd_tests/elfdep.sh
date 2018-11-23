#!/bin/bash
set -e
set -x

cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
source set_env_vars

cd $SECURITY_TRANSFORMS_HOME/libElfDep/test/
./test-elfdep.sh
