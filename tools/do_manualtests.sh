#!/bin/sh

#
# We support two forms of manual tests:
#
# (1) User supplies tests with input/output specifications
#       - these are automatically identified by looking for the command: manual_test_import
#       - coverage information is automatically computed using pin
# (2) User supplies black box regression test suite
#       - coverage information is optionally supplied (format of coverage file: 1 address per line)
#
# Assumption: this script is called from within the subdirectory created as part of ps_analyze.sh
#

name=$1                     # original basename of binary
newname=$2                  # new name of protected binary
test_script=$3              # test script
manual_coverage_file=$4     # coverage file for black box regression test suite

PWD=`pwd`
echo "Running specified test script at: $3 Current working dir is: $PWD coverage file: $4"
echo "Current working dir is: $PWD command issued: $*"

if [ ! -f $test_script ]; then
  echo "Error -- could not find testing cript: $test_script"
  exit 1
else
  mkdir manual_tests 2>/dev/null

  grep manual_test_import $test_script
  if [ $? -eq 0 ]; then
    # execute the script to import manual test cases 
    sh $test_script
  else
    # setup script for black box regression testing
	# currently used with P1/Pn
    cp $test_script manual_test_wrapper
	echo $newname > new_command_name

	# optionally use coverage information (if provided)
	if [ -f $manual_coverage_file ]; then
	  cp $manual_coverage_file manual_coverage.txt
	fi
  fi

  ln -s $name.sh $newname
fi
