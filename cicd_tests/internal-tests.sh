#!/bin/bash 

set -e

cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
source set_env_vars
# run zipr internal tests
cd $ZIPR_HOME/test; scons
for i in *.exe; do ./$i; done


