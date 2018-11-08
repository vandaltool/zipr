cd $CICD_MODULE_WORK_DIR/peasoup_umbrella

set -e
source set_env_vars
cd /tmp
rm -rf cat.rida ped_cat; $PSZ $(which cat) ./cat.rida -c rida=on -s meds_static=off --tempdir ped_cat || true
if [[ ! -x ./cat.rida ]]; then cat ped_ls/logs/*; fi
./cat.rida /dev/null 
