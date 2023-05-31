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
cicd_setup_module_dependency allzp/idapro82sp1py3.git idapro82sp1py3
export IDAROOT=$CICD_MODULE_WORK_DIR/idapro82sp1py3
cicd_setup_module_dependency allzp/idapro82_sdk.git idapro82_sdk
export IDASDK=$CICD_MODULE_WORK_DIR/idapro82_sdk

cd /tmp/peasoup_test
source set_env_vars

sudo ./get-peasoup-packages.sh all
bash -x ./build-all.sh 
bash -x ./postgres_setup.sh

