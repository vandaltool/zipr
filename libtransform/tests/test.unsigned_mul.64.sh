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
# ATTRIBUTE TestName=unsigned_mul.64
# ATTRIBUTE CompilerFlags="-w"

COMPFLAGS="-w"

export IDAROOT=$IDAROOT65
export IDASDK=$IDASDK65

PWD=`pwd`
TESTLOC="${PWD}"
tmp1=$$.tmp.1
tmp2=$$.tmp.2

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

#	cd $TESTLOC
#	rm -f $tmp1 $tmp2 2>/dev/null
#	rm -fr peasoup*unsigned*mul*64*
#	rm -fr unsigned*mul*64*.exe*
#	cd -

	exit $exit_code
}

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN IDAROOT IDASDK PEASOUP_HOME SECURITY_TRANSFORMS_HOME SMPSA_HOME

# path to source
cd $TESTLOC
make unsigned_mul.64.exe

if [ ! $? -eq 0 ]; then
	cleanup 1 "Failed to build"
fi

# test normal results
./unsigned_mul.64.exe 2 4 > $tmp1
./unsigned_mul.64.exe.peasoup 2 4 > $tmp2
diff $tmp1 $tmp2
if [ ! $? -eq 0 ]; then
	cat $tmp1 $tmp2
	cleanup 2 "multiply failed: 2 * 4"
fi

./unsigned_mul.64.exe -2 4 > $tmp1
./unsigned_mul.64.exe.peasoup -2 4 > $tmp2
diff $tmp1 $tmp2
if [ ! $? -eq 0 ]; then
	cat $tmp1 $tmp2
	cleanup 3 "multiply failed: -2 * 4"
fi

./unsigned_mul.64.exe -2 -4 > $tmp1
./unsigned_mul.64.exe.peasoup -2 -4 > $tmp2
diff $tmp1 $tmp2
if [ ! $? -eq 0 ]; then
	cat $tmp1 $tmp2
	cleanup 4 "multiply failed: -2 * -4"
fi

./unsigned_mul.64.exe.peasoup 80000000000 80000000000 > $tmp1
grep "18446744073709551615" $tmp1

if [ ! $? -eq 0 ]; then
	cat $tmp1
	cleanup 5 "saturation failed: 80000000000 80000000000"
fi


cleanup 0 "unsigned_mul.64 test success"
