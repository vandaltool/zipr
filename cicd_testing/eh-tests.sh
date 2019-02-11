#!/bin/bash
set -e
set -x

cd /tmp/peasoup_test
source set_env_vars
cd $CICD_TO_TEST_DIR/peasoup_examples/cpp-examples
cd $PEASOUP_HOME/cpp-examples
./test_quick.sh

