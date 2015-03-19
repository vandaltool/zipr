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
# ATTRIBUTE TestName=ldap_interception
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
make clean all peasoup
if [ ! $? -eq 0 ]; then
	cleanup 1 "Failed to build mysql intercept tests"
fi

APPFW_VERBOSE=1 LDAP_DATA="hacker)(|(objectclass=*)" ./testldapintercept.exe.peasoup 2>$tmp >/dev/null
grep detected $tmp
if [ ! $? -eq 0 ]; then
	echo "Output:"
	cat $tmp
	cleanup 1 "Failed to intercept and/or detect security violation"
fi

APPFW_VERBOSE=1 LDAP_DATA="hacker)(|(objectclass=*)" ./testldapintercept2.exe.peasoup 2>$tmp >/dev/null
grep detected $tmp
if [ ! $? -eq 0 ]; then
	echo "Output:"
	cat $tmp
	cleanup 2 "Failed to intercept and/or detect security violation"
fi

APPFW_VERBOSE=1 LDAP_DATA="hacker)(|(objectclass=*)" ./testldapintercept3.exe.peasoup 2>$tmp >/dev/null
grep detected $tmp
if [ ! $? -eq 0 ]; then
	echo "Output:"
	cat $tmp
	cleanup 4 "Failed to intercept and/or detect security violation"
fi

APPFW_VERBOSE=1 LDAP_DATA="hacker)(|(objectclass=*)" ./testldapintercept4.exe.peasoup 2>$tmp >/dev/null
grep detected $tmp
if [ ! $? -eq 0 ]; then
	echo "Output:"
	cat $tmp
	cleanup 8 "Failed to intercept and/or detect security violation"
fi

cleanup 0 "Successfully tested ldap interception layer"
