#!/bin/bash

IGNORE_RESULTS=

while getopts “i” OPTION
do
     case $OPTION in
         h)
             usage
             exit 1
             ;;
         i)
             IGNORE_RESULTS=1
             ;;
         ?)
			 usage
             exit
             ;;
     esac
done

shift $(( OPTIND-1 ))

if [[ ! -z "$IGNORE_RESULTS" ]]; then
	echo "TEST SCRIPT COVERAGE RUN: running test script ignoring results."
fi

TEST_PROG=$1
BENCH=$2

DUMMY_NAME=DUMMY

BENCH_BASE=`basename $BENCH`
TEST_BASE=`basename $TEST_PROG`

NAME_REGEX=`echo "($BENCH_BASE\|$TEST_BASE\|$ORIG_NAME\|.peasoup\|.ncexe\|.stratafied)" | sed 's/\./\\\./'`

usage()
{
	echo "<script> [-i] <test_prog> <orig_prog>"
}

cleanup()
{
#	echo "empty"
	rm -f test_out test_error test_status orig_out orig_error orig_status $CLEANUP_FILES
}

report_failure()
{
	cleanup
    echo "TEST WRAPPER FAILED"
		#in case of premature termination, terminate any programs still running
	killall a.stratafied
	#ignore the results, and continue. 
	if [[ ! -z "$IGNORE_RESULTS" ]]; then
		return
	fi

    exit 1
}

report_success()
{
	cleanup
    echo "TEST WRAPPER SUCCESS"
    exit 0
}

#$1 is the name of the directory to create, 
#the remaining args are the files to store in the created directory
log_results()
{
	if [[ -z "$LOG_DIR" || ! -d "$LOG_DIR" ]]; then
		return
	fi

	log_name=$1
	shift

	mkdir $LOG_DIR/$log_name

	for file in $@
	do
		cp $file $LOG_DIR/$log_name/.
	done
}

run_test_prog_only()
{
	TIMEOUT=$1
	shift

	cmd_args="$@"

	if [[ "$TEST_PROG" == "" ]]; then
		echo "TEST SCRIPT ERROR: TEST_PROG does not exist, reporting failure"
		report_failure
	fi

	if [[ "$TIMEOUT" -le 0 ]] || [[ ! -z "$IGNORE_RESULTS" ]]; then
		echo "$TEST_PROG $@ >test_out 2>test_error"
		$TEST_PROG "$@" >test_out 2>test_error
	else
		echo "timeout $TIMEOUT $TEST_PROG $@ >test_out 2>test_error"
		timeout $TIMEOUT $TEST_PROG "$@" >test_out 2>test_error
	fi

	status=$?
	echo $status >test_status

	log_name=`echo "TEST_$TEST_PROG $cmd_args" | sed -e 's/ /_/g' -e 's/\//#/g'`
	log_results $log_name test_out test_error test_status 

	return $status
}

run_bench_prog_only()
{
	TIMEOUT=$1
	shift

	cmd_args="$@"

	#ignore the results, and continue. 
	if [[ ! -z "$IGNORE_RESULTS" ]]; then
		return
	fi

	if [[ "$BENCH" == "" ]]; then
		echo "TEST SCRIPT ERROR: BENCH does not exist, reporting failure"
		report_failure
	fi

	if [[ "$TIMEOUT" -le 0 ]] || [[ ! -z "$IGNORE_RESULTS" ]]; then
		echo "eval $BENCH $@ >orig_out 2>orig_error"
		$BENCH "$@" >orig_out 2>orig_error
	else
		echo "timeout $TIMEOUT $BENCH $@ >orig_out 2>orig_error"
		timeout $TIMEOUT $BENCH "$@" >orig_out 2>orig_error
	fi

	status=$?
	
	echo $status >orig_status

	log_name=`echo "BENCH_$BENCH $cmd_args" | sed -e 's/ /_/g' -e 's/\//#/g'`
	log_results $log_name orig_out orig_error orig_status 

	return $status
}


#diff $1 and $2, do not use the name filtering, and report
#failure if not matching
compare_files_no_filtering()
{
		#ignore the results, and continue. 
	if [[ ! -z "$IGNORE_RESULTS" ]]; then
		return
	fi

	diff $1 $2
	if [ ! "$?" -eq 0 ]; then
		echo "File Comparison Failure"
		report_failure
	fi 
}

#compares orig_status and test_status files
compare_exit_status()
{
	#ignore the results, and continue. 
	if [[ ! -z "$IGNORE_RESULTS" ]]; then
		return
	fi

	diff orig_status test_status

	if [ ! "$?" -eq 0 ]; then
		echo "Exit Status Failure"
		report_failure
	fi 
}

#assumes that orig_status, test_status, orig_error, test_error, orig_out, and test_out
#all exists, and does a comparison of each. 
compare_std_results()
{
	#ignore the results, and continue. 
	if [[ ! -z "$IGNORE_RESULTS" ]]; then
		return
	fi

	diff orig_status test_status

	if [ ! "$?" -eq 0 ]; then
		echo "Exit Status Failure"
		report_failure
	fi 

	filter_prog_name orig_error
	filter_prog_name test_error

	diff orig_error test_error
	if [ ! $? -eq 0 ]; then
		echo "Stderr  Failure"
		report_failure
	fi 

	filter_prog_name orig_out
	filter_prog_name test_out

	diff orig_out test_out
	if [ ! $? -eq 0 ]; then
		echo "run_test Stdout Failure"
		report_failure
	fi 
}

#Runs both TEST_PROG and BENCH and compares stdout stderr and the exit status.
#Arg 1 is the timeout time
#The remaining args are the arguments to pass to both TEST_PROG and BENCH
run_basic_test()
{
	cleanup

	run_test_prog_only "$@"

	#ignore the results, and continue. 
	if [[ ! -z "$IGNORE_RESULTS" ]]; then
		return
	fi

	run_bench_prog_only "$@"

	compare_std_results
	cleanup
}

filter_prog_name()
{
	cat $1 | sed -r -e "s|[^[:space:]]*$NAME_REGEX|$DUMMY_NAME|g" >tmp

	if [[ ! -z "$DELETE_FILTER" ]]; then
		cat tmp | sed -r -e "/($DELETE_FILTER)/d">$1
	else
		cat tmp>$1
	fi
#	rm tmp
}
