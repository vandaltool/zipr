#!/bin/bash
set -e
set -x

cd cd /tmp/peasoup_test
source set_env_vars

cd $SECURITY_TRANSFORMS_HOME/libIRDB-elfdep/test/
./test-elfdep.sh
