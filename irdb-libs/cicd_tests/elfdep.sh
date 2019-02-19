#!/bin/bash
set -e
set -x

cd $CICD_MODULE_WORK_DIR/irdblibs_umbrella
source set_env_vars

cd $SECURITY_TRANSFORMS_HOME/libIRDB-elfdep/test/
./test-elfdep.sh
