#!/bin/bash
set -e
set -x

cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
source set_env_vars
cd /tmp
rm -rf ls.rida ped_ls; $PSZ /bin/ls ./ls.rida -c rida=on -s meds_static=off --tempdir ped_ls || true
if [[ ! -x ./ls.rida ]]; then cat ped_ls/logs/*; fi
rm -rf ped_ls
./ls.rida
