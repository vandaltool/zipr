#!/bin/sh -x

# Assumptions:
# 	$1 is the full pathname to output file

# For PEASOUP, Required XML fields are
# name - name of the test
# host - name of the host where the test was run
# project - project name
# date_time - date time in specific format date +%FT%R:%S
# key_value pairs, any number
#   may include result, user, host platform, build platform

# Fixed attributes
# ATTRIBUTE ModDep=strata
# ATTRIBUTE ModDep=diablo_toolchain
# ATTRIBUTE ModDep=stratafier
# ATTRIBUTE ModDep=idaproCur
# ATTRIBUTE ModDep=idaproCur_sdk
# ATTRIBUTE ModDep=peasoup_examples
# ATTRIBUTE ModDep=security_transforms
# ATTRIBUTE ModDep=SMPStaticAnalyzer
# ATTRIBUTE TestsWhat=lang_C
# ATTRIBUTE TestsWhat=strata
# ATTRIBUTE TestsWhat=IntegerOverflow
# ATTRIBUTE OS=linux
# ATTRIBUTE Compiler=gcc
# ATTRIBUTE Arch=x86_64
# ATTRIBUTE TestName=trunc.64.8.unknown
# ATTRIBUTE CompilerFlags="-w"

COMPFLAGS="-w"

if [ -z $IDAROOT ]; then
  export IDAROOT=$IDAROOT65
fi

if [ -z $IDASDK ]; then
  export IDASDK=$IDASDK65
fi

PWD=`pwd`
TESTLOC="${PWD}"
tmp1=$$.tmp.1
tmp2=$$.tmp.2
BINARY=trunc.64.8.unknown.exe
BINARY_PEASOUP=$BINARY.peasoup

outfile=$1

cleanup()
{
	exit_code=$1
	shift
	msg=$*

	if [ $exit_code -eq 0 ]; then 
		report_test_success $outfile "$msg"
	else
		report_test_failure $outfile "Intermediate step failed, exit code is $exit_code, msg='$msg'"
	fi

	cd $TESTLOC
	rm -f $tmp1 $tmp2 2>/dev/null
	rm -fr peasoup*$BINARY*.*
	cd -

	exit $exit_code
}

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN IDAROOT IDASDK PEASOUP_HOME SECURITY_TRANSFORMS_HOME SMPSA_HOME

# path to source
cd $TESTLOC
rm $BINARY
make $BINARY

if [ ! $? -eq 0 ]; then
	cleanup 1 "Failed to build"
fi

# test normal results
./$BINARY 100 > $tmp1
./$BINARY_PEASOUP 100 > $tmp2
diff $tmp1 $tmp2
if [ ! $? -eq 0 ]; then
	cat $tmp1 $tmp2
	cleanup 2 "false positive detected"
fi

# make sure results differ
./$BINARY 888888888888 > $tmp1
./$BINARY_PEASOUP 888888888888 > $tmp2
diff $tmp1 $tmp2
if [ $? -eq 0 ]; then
	cat $tmp1 $tmp2
	cleanup 8 "failed to detect/saturate truncation"
fi

# check saturation value
grep "= 127"  $tmp2
if [ ! $? -eq 0 ]; then
	cat $tmp2
	cleanup 9 "failed to saturate properly"
fi

cleanup 0 "$BINARY test success"
