#/bin/bash 

set -e

# gather info for debugging later, probably not necessary 
pwd
hostname
whoami
env|grep CICD

# puts peasoup_umbrella (and all submodules) in CICD_MODULE_WORK_DIR
cicd_setup_module_dependency allnp/peasoup_umbrella.git


# puts the version of zipr to test in peasoup_umbrella/zipr.
cicd_put_module_in_tree peasoup_umbrella/zipr

# Build/run $PSZ, test result
cd $CICD_MODULE_WORK_DIR/peasoup_umbrella
source set_env_vars
sudo ./get-peasoup-packages.sh all
./build-all.sh
dropdb $PGDATABASE 2>/dev/null || true ; ./postgres_setup.sh

