#/bin/bash 
set -e
set -x

cd /tmp/peasoup_test
source set_env_vars
cd $PEASOUP_HOME/tests
make clean
./test_cmds.sh -l -c rida_p1

