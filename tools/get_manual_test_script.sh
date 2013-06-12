#!/bin/bash 

PROGRAM_NAME=$1
TEST_DIR=$PEASOUP_HOME/tests

found()
{
	echo "found manual test script: $1"
	echo "$1" >get_manual_test_script.log
	exit 0
}

case "$PROGRAM_NAME" in
	"bunzip2")
		
		found $TEST_DIR/bzip/test_script.sh
		;;
	"touch")
		found $TEST_DIR/touch/test_script.sh
		;;
	* )
		echo "get_manual_test_script.sh: no matching test script found"
		exit 1
		;;
esac