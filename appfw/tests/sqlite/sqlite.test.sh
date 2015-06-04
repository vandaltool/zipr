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
# ATTRIBUTE TestsWhat=commandinjection
# ATTRIBUTE OS=linux
# ATTRIBUTE Compiler=gcc
# ATTRIBUTE Arch=x86_64
# ATTRIBUTE TestName=sqlite
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

#	cd $TESTLOC
#	rm -f $tmp 2>/dev/null
#	make clean
#	cd -
#
# 	rm -f $tmp
	exit $exit_code
}

if [ ! -z $IDAROOT65 ]; then
  export IDAROOT=$IDAROOT65
fi

if [ ! -z $IDASDK65 ]; then
  export IDASDK=$IDASDK65
fi

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN IDAROOT IDASDK PEASOUP_HOME SECURITY_TRANSFORMS_HOME

cd $TESTLOC

# let's do it all over again 
# but this time with the full Peasoup suite of protection
make clean peasoup
if [ ! $? -eq 0 ]; then
	cleanup 6 "Failed to build sqlite Peasoup tests"
fi
sqlite3 $dbname < ./setup.sql

# good query
echo "Testing good query"
rm -f $tmp 2>/dev/null
QUERY_DATA=David ./testsqlite.peasoup > $tmp 2>&1
grep -i "last = Hyde" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 7 "False positive detected: query for testsqlite.peasoup should have succeeded"
fi

echo "Testing attack detection"
rm -f $tmp
QUERY_DATA="David' or '0'='0" ./testsqlite.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 8 "False negative detected: attack query for testsqlite.peasoup should have failed"
fi

#
# test parameterized stmt
#
echo "Testing parameterized statement"
rm -f $tmp
./testsqlite.pstmt.peasoup > $tmp 2>&1
grep -i "Hyde" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 9 "False positive detected: query for testsqlite.peasoup should have succeeded"
fi

echo "Testing attack on parameterized statement"
rm -f $tmp
QUERY_DATA=" or 1 = 1 -- " ./testsqlite.pstmt.peasoup > $tmp 2>&1
grep -i "injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 10 "False negative detected: should have detected attack on parameterized stmt"
fi

cleanup 0 "Successfully detected sqlite SQL Injection on Full peasoup"
