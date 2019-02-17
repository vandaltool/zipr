#!/bin/bash 

set -e
set -x

cd $CICD_MODULE_WORK_DIR/zipr_umbrella
source set_env_vars
# run zipr internal tests
cd $ZIPR_HOME/test; scons
for i in *.exe; do ./$i; done


