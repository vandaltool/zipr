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
# ATTRIBUTE TestsWhat=peasoup_end2end
# ATTRIBUTE OS=linux
# ATTRIBUTE Compiler=gcc
# ATTRIBUTE Arch=x86_32

# ATTRIBUTE TestName=smartfuzz
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
	cleanup 1 "Failed to build postgres tests"
fi


#
# testpg1.exe.peasoup
#

# test good queries
rm -f $tmp 2>/dev/null
./testpg1.exe.peasoup bob > $tmp 2>&1
grep -i query $tmp | grep -i success
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 2 "False positive detected: query for testpg1.exe.peasoup should have succeeded"
fi

rm -f $tmp
./testpg1.exe.peasoup "select * from xyz" > $tmp 2>&1
grep -i query $tmp | grep -i success
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 3 "False positive detected: query for testpg1.exe.peasoup should have succeeded"
fi

# test attack queries
rm -f $tmp
./testpg1.exe.peasoup "' or 1 = 1;--" > $tmp 2>&1
grep -i "sql injection" $tmp | grep -i detected
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 4 "False negative detected: attack query for testpg1.exe.peasoup should have been detected"
fi

rm -f $tmp
./testpg1.exe.peasoup "' and /* */ 1 = 1 /* */; /*--*/" > $tmp 2>&1
grep -i "sql injection" $tmp | grep -i detected
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 5 "False negative detected: attack query for testpg1.exe.peasoup should have been detected"
fi

rm -f $tmp
./testpg1.exe.peasoup "%' or 1 = 1; -- select *" > $tmp 2>&1
grep -i "sql injection" $tmp | grep -i detected
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 6 "False negative detected: attack query for testpg1.exe.peasoup should have been detected"
fi

#
# testpg2.exe.peasoup
#
psql -f ./teardown.sql 2>/dev/null # in case we have remnmants from a previous testing run
psql -f ./setup.sql

# good query
rm -f $tmp
./testpg2.exe.peasoup David > $tmp 2>&1
grep -i "David Hyde" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 7 "False positive detected: query for testpg2.exe.peasoup should have succeeded"
fi

# attack query
rm -f $tmp
./testpg2.exe "David' or '0'='0" > $tmp 2>&1
grep -i William $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 7 "False negative detected: attack query for testpg2.exe.peasoup should have failed"
fi

psql -f ./teardown.sql

#
# testpg4.exe.peasoup
# test multi-statement queries
#
rm -f $tmp
./testpg4.exe.peasoup "bob" > $tmp 2>&1
grep -i "sql injection" $tmp | grep -i detected
if [ $? -eq 0 ]; then
	cat $tmp
	cleanup 8 "False positive detected: there should be no SQL injections here"
fi

cleanup 0 "Successfully detected Postgres SQL Injection"
