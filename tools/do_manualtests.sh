#!/bin/sh

name=$1
newname=$2
test_script=$3

#
# $1 contains the user-supplied test input/output specification script
# assumption: this script is called from within the subdirectory created as part of
# ps_analyze.sh
#
PWD=`pwd`
echo "Running specified test script at: $3 Current working dir is: $PWD"

if [ ! -f $test_script ]; then
  echo "Error -- could not find manual input/output testing setup script: $1"
  exit 1
else
  mkdir manual_tests 2>/dev/null

  grep manual_test_import $test_script
  if [ $? -eq 0 ]; then
    sh $test_script
  else
    cp $test_script manual_test_wrapper
	echo $newname > new_command_name
    ln -s $name.sh $newname
  fi

fi
