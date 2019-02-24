#!/bin/bash
set -e
set -x

cd /tmp/peasoup_test
source set_env_vars
cd $CICD_TO_TEST_DIR/peasoup_examples/tests
cd $PEASOUP_HOME/tests
make clean; 
./test_cmds.sh -c kill_deads_rida -l -a "tcpdump"
./test_cmds.sh -c kill_deads -l -a "bzip2" 

