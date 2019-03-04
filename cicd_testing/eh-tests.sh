#!/bin/bash
set -e
set -x

cd /tmp/peasoup_test
source set_env_vars
cd $CICD_TO_TEST_DIR/examples/cpp-examples
./test_quick_zipr_only.sh

