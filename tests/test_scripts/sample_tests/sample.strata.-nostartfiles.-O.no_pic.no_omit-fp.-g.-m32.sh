#!/bin/sh -x

# Assumptions:
# 	$1 is the full pathname to output file


# For PEASOUP Test Data Manager, Required XML fields are
# name - name of the test
# host - name of the host where the test was run
# project - project name
# date_time - date time in specific format date +%FT%R:%S
# key_value pairs, any number
#   may include result, user, host platform, build platform


# Fixed attributes
# ATTRIBUTE ModDep=diablo_toolchain
# ATTRIBUTE ModDep=binutils-2.19
# ATTRIBUTE ModDep=idaproCur
# ATTRIBUTE ModDep=idaproCur_sdk
# ATTRIBUTE ModDep=peasoup_examples
# ATTRIBUTE ModDep=security_transforms
# ATTRIBUTE ModDep=SMPStaticAnalyzer
# ATTRIBUTE ModDep=strata
# ATTRIBUTE ModDep=stratafier
# ATTRIBUTE ModDep=zipr
# ATTRIBUTE ModDep=zipr_sdk
# ATTRIBUTE ModDep=zipr_callbacks
# ATTRIBUTE ModDep=zipr_push64_reloc_plugin
# ATTRIBUTE ModDep=zipr_scfi_plugin
# ATTRIBUTE ModDep=zipr_install
# ATTRIBUTE TestsWhat=lang_C
# ATTRIBUTE TestsWhat=strata
# ATTRIBUTE OS=linux
# ATTRIBUTE Compiler=g++
# ATTRIBUTE BenchmarkSuite=peasoup_micro_tests
# ATTRIBUTE CompilerFlags="-nostartfiles -O     -g -m32"


# Filled in by test generator
# ATTRIBUTE Arch=x86_32
# ATTRIBUTE TestName="ps_micro_test sample"
# ATTRIBUTE BenchmarkName=sample

BENCHNAME=sample
COMPFLAGS="-nostartfiles -O     -g -m32"

outfile=$1

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$STRATA/lib/

cleanup()
{
	exit_code=$1
	shift
	msg=$*

	if [ $exit_code -ne 0 ]; then 
		report_test_failure $outfile "Intermediate step failed, exit code is $exit_code, msg='$msg'"
	fi

	if [ -z $NO_CLEANUP ]; then
		rm -rf peasoup_executable*

		# remove intermediate results directory
		rm -rf test_output
		rm -rf ${test_binary} ${analyzed_binary}
	fi

	exit $exit_code
}

# suck in utils
. ${TEST_HARNESS_HOME}/test_utils.sh || cleanup 1 "Cannot source utils file"

assert_test_args $*
assert_test_env $outfile STRATAFIER STRATA TOOLCHAIN STRATAFIER_OBJCOPY THTTPD_HOME THTTPD_INSTALL ZIPR_HOME ZIPR_INSTALL ZIPR_CALLBACKS ZIPR_SDK ZIPR_SCFI_PLUGIN PEASOUP_HOME SMPSA_HOME IDAROOTCUR IDASDKCUR SECURITY_TRANSFORMS_HOME

# set up IDA environment variables
export IDAROOT=$IDAROOTCUR
export IDASDK=$IDASDKCUR

# set up IRDB variables
export PGHOST=127.0.0.1
export PGUSER=$USER
export PGPORT=5432
export PGDATABASE=peasoup_$USER

# path to source
testloc=${PEASOUP_HOME}/tests/test_scripts

# compile  
# g++ -w $start_files $arch $opt $pic $fp $debug test.cpp foo.cpp
# g++ -w $start_files $arch $opt $pic $fp $debug sample.cpp

# figure this out!!!
test_binary=orig.$$
analyzed_binary=${test_binary}.strata

g++ -w $COMPFLAGS ${testloc}/$BENCHNAME.cpp -o ${test_binary} || cleanup 2 "g++ failed"

# sanity check the compile
if [ ! -f ${test_binary} ]; then cleanup 3 "Failed to create ${test_binary}"; fi

# run the un-analyzed binary and save for comparison
(echo "goodpass" && cat) | ./${test_binary} > test_output/${test_binary}.out
orig_retval=$?


#  analyze
$PEASOUP_HOME/tools/ps_analyze.sh ${test_binary}  ${analyzed_binary} --step ilr=on  --backend strata || cleanup 4 "ps_analyze failed"

# run program 
(echo "goodpass" && cat) | ./${analyzed_binary} > test_output/${analyzed_binary}.out
retval=$?

# check the retvals
if [ ${orig_retval} -ne ${retval} ]; then
	echo "original retval: ${orig_retval} does not match analyzed retval: ${analyzed_retval}" 
fi

# check the output values
diff test_output/${test_binary}.out test_output/${analyzed_binary}.out
if [ $? -ne 0 ]; then
	echo "The original and analyzed outputs differ"
fi

# report
report_test_result $outfile $retval "Return value of protected run was: $retval" || cleanup 5 "Reporting failed?"

# cleanup
cleanup 0 "Success!"
