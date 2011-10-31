#!/bin/bash 

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
# ATTRIBUTE TestsWhat=interoverflow
# ATTRIBUTE TestsWhat=shared_lib
# ATTRIBUTE OS=linux
# ATTRIBUTE BenchmarkSuite=IntegerOverflow
# ATTRIBUTE Compiler=gcc
# ATTRIBUTE Arch=x86_32

# Filled in by test generator
# ATTRIBUTE TestName="addsub.unsigned+int.int.c"
# ATTRIBUTE BenchmarkName=addsub.unsigned+int.int
# ATTRIBUTE CompilerFlags="-w"

BENCHNAME=addsub.unsigned+int.int
COMPFLAGS="-w"
ARG1_TYPE="unsigned+int"
ARG2_TYPE="int"

# export general IDA pro environment vars
export IDAROOT=$IDAROOT61
export IDASDK=$IDASDK61

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

	rm -f $orig "peasoup_executable_directory.*$orig*.exe.*" 
	exit $exit_code
}

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN STRATAFIER_OBJCOPY IDAROOT IDASDK 

# path to source
testloc=`pwd`

orig=$BENCHNAME.$$.exe

# compile
gcc ${testloc}/$BENCHNAME.c  $COMPFLAGS -o $orig || cleanup 2 "gcc failed"

# sanity check compile
if [ ! -f $orig ]; then cleanup 3 "Failed to create $orig"; fi

$PEASOUP_HOME/tools/ps_analyze.sh $orig $orig.protected --step ilr=off --step p1transform=off --step concolic=off --step isr=off

${testloc}/$orig.protected 0 0 | grep -i "overflow detected" | grep -i "add"
if [ $? -eq 0 ]; then
        cleanup 4 "False positive detected"
fi

MAX_INT=2147483647
MAX_UINT=4294967295

case $ARG1_TYPE in
	"int")
		arg1=$MAX_INT
		arg2=$MAX_INT
		;;
	"unsigned+int")
		arg1=$MAX_UINT
		arg2=$MAX_UINT
		;;
esac

${testloc}/$orig.protected $arg1 $arg2 | grep -i "overflow detected" | grep -i "add"
if [ ! $? -eq 0 ]; then
	cleanup 5 "Did not detect add/sub overflow for arguments: $arg1($ARG1_TYPE) $arg2($ARG2_TYPE)"
else
	cleanup 0 "Success! add/sub overflow detected"
fi

