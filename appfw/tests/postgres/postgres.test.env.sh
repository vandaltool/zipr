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
# ATTRIBUTE TestsWhat=lang_C
# ATTRIBUTE TestsWhat=strata
# ATTRIBUTE TestsWhat=commandinjection
# ATTRIBUTE TestsWhat=peasoup_end2end
# ATTRIBUTE OS=linux
# ATTRIBUTE Compiler=gcc
# ATTRIBUTE Arch=x86_32

# ATTRIBUTE TestName=postgres_via_env
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

#	cd $TESTLOC
# 	rm -f $tmp 2>/dev/null
#	make clean
#	cd -

	exit $exit_code
}

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN IDAROOT IDASDK PEASOUP_HOME SECURITY_TRANSFORMS_HOME

# path to source
cd $TESTLOC
make clean peasoup.env
if [ ! $? -eq 0 ]; then
	cleanup 1 "Failed to build postgres tests"
fi

psql -f ./teardown.sql 2>/dev/null # in case we have remnants from a previous testing run
psql -f ./setup.sql

#
# testpg1.exe.env.peasoup
#

# test good queries
rm -f $tmp 2>/dev/null
QUERY_DATA="bob" ./testpg1.exe.env.peasoup > $tmp 2>&1
grep -i query $tmp | grep -i success
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 2 "False positive detected: query for testpg1.exe.env.peasoup should have succeeded"
fi

rm -f $tmp
QUERY_DATA="select * from xyz" ./testpg1.exe.env.peasoup > $tmp 2>&1
grep -i query $tmp | grep -i success
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 3 "False positive detected: query for testpg1.exe.env.peasoup should have succeeded"
fi

# test attack queries
APPFW_VERBOSE=1 QUERY_DATA="' or 1 = 1;--" ./testpg1.exe.env.peasoup > $tmp 2>&1
grep -i "tautology" $tmp 
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 4 "False negative detected: attack query for testpg1.exe.env.peasoup should have been detected"
fi

rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA="' and /* */ 1 = 1 /* */; /*--*/" ./testpg1.exe.env.peasoup > $tmp 2>&1
grep -i "sql injection" $tmp 
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 5 "False negative detected: attack query for testpg1.exe.env.peasoup should have been detected"
fi

rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA="%' or 1 = 1; -- select *" ./testpg1.exe.env.peasoup > $tmp 2>&1
grep -i "sql injection" $tmp 
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 6 "False negative detected: attack query for testpg1.exe.env.peasoup should have been detected"
fi

#
# testpg2.exe.env.peasoup
#

# test good queries
rm -f $tmp 2>/dev/null
APPFW_VERBOSE=1 QUERY_DATA="bob" ./testpg2.exe.env.peasoup > $tmp 2>&1
grep -i --binary-files=text query $tmp | grep -i success
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 7 "False positive detected: query for testpg2.exe.env.peasoup should have succeeded"
fi

rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA="select * from xyz" ./testpg2.exe.env.peasoup > $tmp 2>&1
grep -i --binary-files=text query $tmp | grep -i success
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 8 "False positive detected: query for testpg2.exe.env.peasoup should have succeeded"
fi

# test attack queries
rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA="' or 1 = 1;--" ./testpg2.exe.env.peasoup > $tmp 2>&1
grep -i "sql injection" $tmp 
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 9 "False negative detected: attack query for testpg2.exe.env.peasoup should have been detected"
fi

rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA="' and /* */ 1 = 1 /* */; /*--*/" ./testpg2.exe.env.peasoup > $tmp 2>&1
grep -i "sql injection" $tmp 
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 10 "False negative detected: attack query for testpg2.exe.env.peasoup should have been detected"
fi

rm -f $tmp
APPFW_VERBOSE=1 QUERY_DATA="%' or 1 = 1; -- select *" ./testpg2.exe.env.peasoup > $tmp 2>&1
grep -i "sql injection" $tmp 
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 11 "False negative detected: attack query for testpg2.exe.env.peasoup should have been detected"
fi

# test prepared stmt
rm -fr $tmp
./testpg2.pstmt.exe.peasoup >$tmp 2>&1
grep -i "query success" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 12 "False positive detected: prepared stmt didn't work"
fi

rm -fr $tmp
APPFW_VERBOSE=1 QUERY_DATA=" or 1 = 1 " ./testpg2.pstmt.exe.peasoup >$tmp 2>&1
grep -i "sql injection" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 13 "False negative detected: didn't detect injection in prepared statement"
fi

psql -f ./teardown.sql
cleanup 0 "Successfully detected Postgres SQL Injection"
