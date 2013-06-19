#!/bin/sh

# BED script to run both manual and conclic tests

TOP_LEVEL=`pwd`
ORIG_PROGRAM=$TOP_LEVEL/a.ncexe
MANUAL_TEST_SCRIPT=$TOP_LEVEL/manual_test_wrapper
CONCOLIC=$TOP_LEVEL/concolic.files_a.stratafied_0001

variantid=$1
aspri=$2
bspri=$3

P1_DIR=p1.xform/$fname

#generate the bspri code
$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $aspri $bspri $TOP_LEVEL/stratafier.o.exe $TOP_LEVEL/libstrata.so.symbols
status=$?
if [ ! $status -eq 0 ]; then
  echo "BED: spasm error -- spasm exited with non-zero status ($status)"
  exit 3

  #I am not sure this condition can happen anymore if spasm doesn't fail, but
  #to keep this code backward compatible I am including this extra check
  elif [ ! -f $bspri ]; then
	echo "BED: spasm error -- no bspri file produced"
	exit 2
fi

$PEASOUP_HOME/tools/fast_spri.sh $bspri $TOP_LEVEL/a.irdb.fbspri

NAME=`cat new_command_name`
TRANSFORMED_PROGRAM="$TOP_LEVEL/$NAME"

echo "Generating hashes"
$STRATA_HOME/tools/preLoaded_ILR/generate_hashfiles.exe $TOP_LEVEL/a.irdb.fbspri
$PEASOUP_HOME/tools/generate_relocfile.sh $TOP_LEVEL/a.irdb.fbspri

#only do the manual tests if the manual_test_wrapper script exists
if [ -f $MANUAL_TEST_SCRIPT ]; then
	echo "Running manual tests: $MANUAL_TEST_SCRIPT $TRANSFORMED_PROGRAM $ORIG_PROGRAM"
	eval $MANUAL_TEST_SCRIPT $TRANSFORMED_PROGRAM $ORIG_PROGRAM
	status=$?

	if [ $status -eq 0 ]; then
		echo "Manual test script success"
	else
		echo "Manual test script failure"
		exit $status
	fi

	
fi

#only do concolic tests if the concolic directory was created. 
if [ -d $CONCOLIC ]; then
	echo "Running concolic tests: ps_validate.sh $TOP_LEVEL/a.stratafied $TOP_LEVEL/a.irdb.fbspri $CONCOLIC >ps_validate.out 2>ps_validate.err"
	$PEASOUP_HOME/tools/ps_validate.sh  $TOP_LEVEL/a.stratafied $TOP_LEVEL/a.irdb.fbspri $CONCOLIC >ps_validate.out 2>ps_validate.err
	status=$?
	if [ $status -eq 0 ]; then
		echo "Concolic test success"
	else
		echo "Conclic test failure"
		exit $status
	fi
fi

echo "BED TEST SUCCESS"

exit 0