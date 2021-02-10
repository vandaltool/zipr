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
cicd_setup_module_dependency allzp/idapro71.git ida71
export IDAROOT=$CICD_MODULE_WORK_DIR/ida71
cicd_setup_module_dependency allzp/idapro71_sdk.git ida71_sdk
export IDASDK=$CICD_MODULE_WORK_DIR/ida71_sdk

sudo ./get-peasoup-packages.sh all
bash -x ./build-all.sh 
bash -x ./postgres_setup.sh

