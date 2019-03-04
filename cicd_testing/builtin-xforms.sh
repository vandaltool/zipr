#!/bin/bash
set -e
set -x

cd /tmp/peasoup_test
source set_env_vars
cd $PEASOUP_HOME/tests
make clean; 
./test_cmds.sh -c "fix_calls_rida rida_p1 fix_calls_ida" -l -a "bzip2 tcpdump"
