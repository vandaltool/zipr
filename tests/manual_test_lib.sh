#!/bin/bash

#Register to the "lop level shell" so I can kill it if 
#a subshell is spawned and attempts to exit. 
export TOP_PID=$$
#exit 1 if a termination signal is given. 
trap "exit 1" TERM

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
	rm -rf test_out test_error test_status orig_out orig_error orig_status $CLEANUP_FILES
}

report_failure()
{
	cleanup
    echo "TEST WRAPPER FAILED"
		#in case of premature termination, terminate any programs still running
    # if there is a comment, print it also
    if [[ "$1" != "" ]]; then
        echo "FAILURE Message:  $1"
    fi

	killall a.stratafied 2>/dev/null
	#ignore the results, and continue. 
	if [[ ! -z "$IGNORE_RESULTS" ]]; then
		return
	fi

	#normally you could call exit directly but if the test script uses 
	#a loop or something of the sort, a subshell may be spawned.
	#exit in this case will only kill the subshell. 
    kill -s TERM $TOP_PID
}

report_success()
{
	cleanup
    echo "TEST WRAPPER SUCCESS"

    # if there is a comment, print it also
    if [[ "$1" != "" ]]; then
        echo "SUCCESS Message:  $1"
    fi

    exit 0
}

#$1 is the name of the directory to create, 
#the remaining args are the files to store in the created directory
log_results()
{
	if [[ -z "$LOG_DIR" || ! -d "$LOG_DIR" ]]; then
		return
	fi

	echo logging:
	for i in $2 $3 $4; 
	do
		echo $i
		cat $i
	done
	

	log_name=$1
	shift

	mkdir -p $LOG_DIR/$log_name

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

# this script takes 3 arguments:
# $1 is the TIMEOUT value
# $2 is the name of the binary to kill
# $3 is the full path to the script which contains commands to send to the server
# $@ is the rest of commandline for invoking the test program
run_server_test_prog_only() 
{
	TIMEOUT=$1
	shift

    # binary name for making call to killallProcess
    BIN_NAME=$1
    shift

    # script containing requests to the server
    TEST_SCRIPT=$1
    echo TEST_SCRIPT=$TEST_SCRIPT
    shift

	cmd_args="$@"

	if [[ "$TEST_PROG" == "" ]]; then
		report_failure "TEST SCRIPT ERROR: TEST_PROG does not exist, reporting failure"
    else
        echo
        echo TEST_PROG=$TEST_PROG
	fi

    # first make sure that there are not other instances of the server running
    echo Killing any pre-existing copies
    killall $BIN_NAME
    sleep 2

    # start the server
	if [[ "$TIMEOUT" -le 0 ]] || [[ ! -z "$IGNORE_RESULTS" ]]; then
		echo "$TEST_PROG $@ >test_out 2>test_error"
		$TEST_PROG "$@" >test_out 2>test_error
	else
		echo "timeout $TIMEOUT $TEST_PROG $@ >test_out 2>test_error"
		timeout $TIMEOUT $TEST_PROG "$@" >test_out 2>test_error
	fi

    # grab the status before killing
	status=$?
	echo $status >test_status

    # run the script containing commands that sends requests to the server
    # this takes a single argument that indicates whether it is test or orig
    if [[ -e "$TEST_SCRIPT" ]]; then
        bash  $TEST_SCRIPT "test"  > throw_away/test_script_test_run_out 2> throw_away/test_script_test_run_error
        echo $? > throw_away/test_script_test_run_status
    else
        report_failure "$TEST_SCRIPT is not a valid filepath to a test script"
    fi

    # kill the server
    echo "after running test script:  Killing $TEST_PROG"
    killall $BIN_NAME
    sleep 2

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

# This script is a version of run_bench_prog_only, but for servers
#   Where the server has to be started, then requests need to be sent to it
#   separately
# $1 is the TIMEOUT value
# $2 is the binary name for kill process command
# $3 is the full path to the script which contains commands to send to the server
# $@ is the rest of commandline for invoking the test program
run_server_bench_prog_only()
{
	TIMEOUT=$1
	shift

    BIN_NAME=$1
    shift

    TEST_SCRIPT=$1
    echo TEST_SCRIPT=$TEST_SCRIPT
    shift

	cmd_args="$@"

	#ignore the results, and continue. 
	if [[ ! -z "$IGNORE_RESULTS" ]]; then
		return
	fi

	if [[ "$BENCH" == "" ]]; then
		report_failure "TEST SCRIPT ERROR: BENCH does not exist, reporting failure"
	fi

    # first make sure that there are not other instances of the server running
    echo "Killing any pre-existing instances of $BIN_NAME"
    killall $BIN_NAME
    sleep 2

	if [[ "$TIMEOUT" -le 0 ]] || [[ ! -z "$IGNORE_RESULTS" ]]; then
		echo "eval $BENCH $@ >orig_out 2>orig_error"
		$BENCH "$@" >orig_out 2>orig_error
	else
		echo "timeout $TIMEOUT $BENCH $@ >orig_out 2>orig_error"
		timeout $TIMEOUT $BENCH "$@" >orig_out 2>orig_error
	fi

	status=$?
	echo $status >orig_status

    # run the script containing commands that sends requests to the server
    # test takes single argument which indicates whether it is orig or test  
    if [[ -e "$TEST_SCRIPT" ]]; then
        bash  $TEST_SCRIPT "orig"  > throw_away/test_script_run_bench_out 2> throw_away/test_script_run_bench_error
        echo $?  > throw_away/test_script_run_bench_status
    else
        report_failure "$TEST_SCRIPT is not a valid filepath to a test script"
    fi


    # kill the server
    echo "Killing $BENCH after running test: $TEST_SCRIPT"
    killall $BIN_NAME
    sleep 2

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

show_log()
{
	echo "=== Show content of $1 ==="
	cat $1
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
		if [ ! -z "$TEST_VERBOSE" ]; then
			show_log orig_out
			show_log test_out
			show_log orig_error
			show_log test_error
		fi
		report_failure
	fi 

	filter_prog_name orig_error
	filter_prog_name test_error

	diff orig_error test_error
	if [ ! $? -eq 0 ]; then
		echo "Stderr  Failure"
		if [ ! -z "$TEST_VERBOSE" ]; then
			show_log orig_error
			show_log test_error
		fi
		report_failure
	fi 

	filter_prog_name orig_out
	filter_prog_name test_out

	diff orig_out test_out
	if [ ! $? -eq 0 ]; then
		echo "run_test Stdout Failure"
		report_failure
		if [ ! -z "$TEST_VERBOSE" ]; then
			show_log orig_out
			show_log test_out
		fi
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
		cat tmp | sed -r -e "/($DELETE_FILTER)/Id">$1
	else
		cat tmp>$1
	fi
	rm tmp
}
