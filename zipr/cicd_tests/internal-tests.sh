#!/bin/bash 

set -e
set -x

cd $CICD_MODULE_WORK_DIR/zipr_umbrella
source set_env_vars
# run zipr internal tests
cd $ZIPR_HOME/test; scons

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PEASOUP_HOME/irdb-libs/lib
for i in *.exe; do ./$i; done


