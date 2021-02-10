#!/bin/bash
set -e
set -x


# update submodules
git submodule sync --recursive
git submodule update --recursive --init
# gather info for debugging later, probably not necessary 
pwd
hostname
whoami
env|grep CICD

time rsync -a --exclude='.git'  $CICD_TO_TEST_DIR/ /tmp/peasoup_test
cd /tmp/peasoup_test
source set_env_vars
cicd_setup_module_dependency allzp/idapro71.git zipr_umbrella_ida
export IDAROOT=$CICD_MODULE_WORK_DIR/zipr_umbrella_ida
cicd_setup_module_dependency allzp/idapro71_sdk.git zipr_umbrella_idasdk
export IDASDK=$CICD_MODULE_WORK_DIR/zipr_umbrella_idasdk

sudo ./get-peasoup-packages.sh all
bash -x ./build-all.sh 
bash -x ./postgres_setup.sh

