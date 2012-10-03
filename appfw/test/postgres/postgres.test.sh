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
# ATTRIBUTE ModDep=binutils-2.19
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

	cd $TESTLOC
	make clean
	cd -

 	rm -f $tmp
	exit $exit_code
}

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN STRATAFIER_OBJCOPY IDAROOT IDASDK PEASOUP_HOME SECURITY_TRANSFORMS_HOME

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
rm -f $tmp
./testpg1.exe.peasoup bob > $tmp 2>&1
grep -i query $tmp | grep -i success
if [ ! $? -eq 0 ]; then
	cleanup 2 "False positive detected: query for testpg1.exe.peasoup should have succeeded"
fi

rm -f $tmp
./testpg1.exe.peasoup "select * from xyz" > $tmp 2>&1
grep -i query $tmp | grep -i success
if [ ! $? -eq 0 ]; then
	cat $tmp
	cleanup 4 "False positive detected: query for testpg1.exe.peasoup should have succeeded"
fi

# test attack queries
rm -f $tmp
./testpg1.exe.peasoup "' or 1 = 1;--" > $tmp 2>&1
grep -i "sql injection" $tmp | grep -i detected
if [ ! $? -eq 0 ]; then
	cleanup 8 "False negative detected: attack query for testpg1.exe.peasoup should have been detected"
fi

rm -f $tmp
./testpg1.exe.peasoup "' and /* */ 1 = 1 /* */; /*--*/" > $tmp 2>&1
grep -i "sql injection" $tmp | grep -i detected
if [ ! $? -eq 0 ]; then
	cleanup 16 "False negative detected: attack query for testpg1.exe.peasoup should have been detected"
fi

rm -f $tmp
./testpg1.exe.peasoup "%' or 1 = 1; -- select *" > $tmp 2>&1
grep -i "sql injection" $tmp | grep -i detected
if [ ! $? -eq 0 ]; then
	cleanup 32 "False negative detected: attack query for testpg1.exe.peasoup should have been detected"
fi
cleanup 0 "Successfully detected Postgres SQL Injection"
