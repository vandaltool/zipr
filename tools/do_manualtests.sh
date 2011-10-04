#!/bin/sh

#
# $1 contains the user-supplied test input/output specification script
# assumption: this script is called from within the subdirectory created as part of
# ps_analyze.sh
#
PWD=`pwd`
echo "Running specified test script at: $1 Current working dir is: $PWD"

if [ ! -f $1 ]; then
  echo "Error -- could not find manual input/output testing setup script: $1"
  exit 1
else
  mkdir manual_tests 2>/dev/null
  sh $1
fi
