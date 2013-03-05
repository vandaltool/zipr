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
# ATTRIBUTE TestName=smartfuzz
# ATTRIBUTE BenchmarkName=TandE
# ATTRIBUTE CompilerFlags="-w"

COMPFLAGS="-w"

# export general IDA pro environment vars
export IDAROOT=$IDAROOT61
export IDASDK=$IDASDK61

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

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN IDAROOT IDASDK PEASOUP_HOME SECURITY_TRANSFORMS_HOME

# path to source
cd $TESTLOC
make clean peasoup
if [ ! $? -eq 0 ]; then
	cleanup 1 "Failed to build sqlite tests"
fi


#
# testpg2.exe.peasoup
#
dbname=testdata

sqlite3 $dbname < ./teardown.sql 2>/dev/null   # in case we have leftover from previous run
sqlite3 $dbname < ./setup.sql

# good query
rm -f $tmp 2>/dev/null
./testsqlite.exe.peasoup David > $tmp 2>&1
grep -i "last = Hyde" $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 2 "False positive detected: query for testsqlite.exe.peasoup should have succeeded"
fi

rm -f $tmp
./testsqlite.exe "David' or '0'='0" > $tmp 2>&1
grep -i William $tmp
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 3 "False negative detected: attack query for testsqlite.exe.peasoup should have failed"
fi

sqlite3 $dbname < ./teardown.sql

cleanup 0 "Successfully detected sqlite SQL Injection"
