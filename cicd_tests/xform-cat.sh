cd $CICD_MODULE_WORK_DIR/peasoup_umbrella

set -e
set -x

source set_env_vars
cd /tmp
rm -rf cat.rida ped_cat; 
$PSZ $(which cat) ./cat.rida -c rida=on -s meds_static=off -c p1transform=on --tempdir ped_cat || true
if [[ ! -x ./cat.rida ]]; then cat ped_cat/logs/*; fi
./cat.rida /dev/null 

