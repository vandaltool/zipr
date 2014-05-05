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
# ATTRIBUTE ModDep=idapro65
# ATTRIBUTE ModDep=idapro65_sdk
# ATTRIBUTE ModDep=peasoup_examples
# ATTRIBUTE ModDep=security_transforms
# ATTRIBUTE ModDep=SMPStaticAnalyzer
# ATTRIBUTE TestsWhat=lang_C
# ATTRIBUTE TestsWhat=strata
# ATTRIBUTE TestsWhat=IntegerOverflow
# ATTRIBUTE OS=linux
# ATTRIBUTE Compiler=gcc
# ATTRIBUTE Arch=x86_64
# ATTRIBUTE TestName=signed_add.64
# ATTRIBUTE CompilerFlags="-w"

COMPFLAGS="-w"

export IDAROOT=$IDAROOT65
export IDASDK=$IDASDK65

PWD=`pwd`
TESTLOC="${PWD}"
tmp1=$$.tmp.1
tmp2=$$.tmp.2

outfile=$1

export IDAROOT=$IDAROOT65
export IDASDK=$IDASDK65

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

#
#	cd $TESTLOC
#	rm -f $tmp1 $tmp2 2>/dev/null
#	rm -fr peasoup*signed*add*64*
#	rm -fr signed*add*64*.exe*
#	cd -


	exit $exit_code
}

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN IDAROOT IDASDK PEASOUP_HOME SECURITY_TRANSFORMS_HOME SMPSA_HOME

# path to source
cd $TESTLOC
rm signed_add.64.exe
make signed_add.64.exe

if [ ! $? -eq 0 ]; then
	env
	cleanup 1 "Failed to build"
fi

# test normal results
./signed_add.64.exe 2 4 > $tmp1
./signed_add.64.exe.peasoup 2 4 > $tmp2
diff $tmp1 $tmp2
if [ ! $? -eq 0 ]; then
	cat $tmp1 $tmp2
	cleanup 2 "failed: 2 + 4"
fi

./signed_add.64.exe -2 4 > $tmp1
./signed_add.64.exe.peasoup -2 4 > $tmp2
diff $tmp1 $tmp2
if [ ! $? -eq 0 ]; then
	cat $tmp1 $tmp2
	cleanup 3 "failed: -2 + 4"
fi

./signed_add.64.exe -2 -4 > $tmp1
./signed_add.64.exe.peasoup -2 -4 > $tmp2
diff $tmp1 $tmp2
if [ ! $? -eq 0 ]; then
	cat $tmp1 $tmp2
	cleanup 4 "failed: -2 + -4"
fi

./signed_add.64.exe.peasoup 9223372036854775807 -1 > $tmp1
grep "9223372036854775806" $tmp1
if [ ! $? -eq 0 ]; then
	cat $tmp1
	cleanup 5 "failed: 9223372036854775807 -1"
fi

./signed_add.64.exe.peasoup 9223372036854775806 25 > $tmp1
grep "9223372036854775807" $tmp1
if [ ! $? -eq 0 ]; then
	cat $tmp1
	cleanup 5 "failed to saturate: 9223372036854775806+25"
fi


cleanup 0 "signed_add.64 test success"
