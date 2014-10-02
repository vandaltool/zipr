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
# ATTRIBUTE ModDep=idapro61
# ATTRIBUTE ModDep=idapro61_sdk
# ATTRIBUTE TestsWhat=lang_C
# ATTRIBUTE TestsWhat=strata
# ATTRIBUTE TestsWhat=commandinjection
# ATTRIBUTE OS=linux
# ATTRIBUTE Compiler=gcc
# ATTRIBUTE Arch=x86_32
# ATTRIBUTE TestName=mysql
# ATTRIBUTE BenchmarkName=TandE
# ATTRIBUTE CompilerFlags="-w"

COMPFLAGS="-w"

PWD=`pwd`
TESTLOC="${PWD}"
tmp=$$.tmp

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
	rm -f $tmp 2>/dev/null
	make clean
	cd -

 	rm -f $tmp
	exit $exit_code
}

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN IDAROOT IDASDK PEASOUP_HOME SECURITY_TRANSFORMS_HOME

# path to source
cd $TESTLOC
make clean peasoup
if [ ! $? -eq 0 ]; then
	cleanup 1 "Failed to build mysql intercept tests"
fi

./testintercept.exe.peasoup foobar > $tmp 2>&1

#
# test interception of mysql_connect
#

rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA="David' or '0'='0"  ./testintercept.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 2 "False negative detected: should have intercepted and stopped attack query"
fi

#
# test interception of mysql_real_connect
#
rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA="David' or '0'='0"  ./testintercept2.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 3 "False negative detected: should have intercepted and stopped attack query"
fi

#
# test interception of mysql_stmt_prepare
#
rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA=" or 1 = 1 " ./testintercept.pstmt.exe.peasoup >$tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 4 "False negative detected: should have intercepted and stopped attack query"
fi

#
# test tautologies
#
APPFW_VERBOSE=1 QUERY_DATA="David' or '0'='0"  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 5 "False negative detected: should have detected tautology"
fi

APPFW_VERBOSE=1 QUERY_DATA="David' or '0'>='0"  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 6 "False negative detected: should have detected tautology"
fi

APPFW_VERBOSE=1 QUERY_DATA="David' or '0'<='0"  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 7 "False negative detected: should have detected tautology"
fi

APPFW_VERBOSE=1 QUERY_DATA="David' or 0=0 "  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 8 "False negative detected: should have detected tautology"
fi

APPFW_VERBOSE=1 QUERY_DATA="David' or 1>=0 "  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 9 "False negative detected: should have detected tautology"
fi

APPFW_VERBOSE=1 QUERY_DATA="David' or 23<=24 "  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 10 "False negative detected: should have detected tautology"
fi

APPFW_VERBOSE=1 QUERY_DATA="David' or 0.5=0.5 "  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 11 "False negative detected: should have detected tautology"
fi

APPFW_VERBOSE=1 QUERY_DATA="David' or 1.25>=1 "  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 12 "False negative detected: should have detected tautology"
fi

APPFW_VERBOSE=1 QUERY_DATA="David' or 23<=24.05 "  ./testtautology.exe.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 13 "False negative detected: should have detected tautology"
fi

cleanup 0 "Successfully tested mysql interception layer"
