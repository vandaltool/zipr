#!/bin/bash

#
# Utilities to assess state of Grace-produced inputs & outputs
#

#
# Did Grace produce at least one valid baseline?
#
# A valid baseline is an input with a set of corresponding outputs, where
# the original run that produces the outputs did not result in a segmentation violation
#
# Returns 0 if Grace did not produce any valid baseline
# Returns 1 if Grace produced at least one valid baseline
#
check_grace_baseline()
{
	CONCOLIC_DIR=$1

	baseline_flag=0
	# do we have at least 1 Grace input that did not generate an 
	# exit code of 139 (segmentation violation)

	for i in `ls $CONCOLIC_DIR/input*.json`
	do
 		input=`basename $i .json`
		input_number=`echo $input | sed "s/input_//"`

		abridged_number=`echo $input_number | sed 's/0*\(.*\)/\1/'`

		#if there is no exit code for the input number, skip for now.
		if [ ! -f "$CONCOLIC_DIR/exit_code.run_$abridged_number.log" ]; then
			echo "do_p1transform.sh: No baseline data for input $input_number, missing exit status"
			continue;
		fi
	echo "check_grace_baseline(): Exit status baseline file: $CONCOLIC_DIR/exit_code.run_$abridged_number.log"
	#if baseline exited with 139, ignore input
	grep 139 $CONCOLIC_DIR/exit_code.run_$abridged_number.log
	if [ $? -eq 0 ]; then
		echo "Baseline exit status was 139 for input $i, ignoring input"
		continue
	fi

	echo "check_grace_baseline(): Found at least one grace produced valid baseline comparison output"
	#at this point we know we have baseline data to compare against
	#we assume that if exit status was produced, baseline information is available
	#if I find one, break the loop
	baseline_flag=`expr $baseline_flag + 1`
	break
	done

	if [ $baseline_flag -eq 0 ]; then
		echo "check_grace_baseline(): no valid baseline comparison output"
	fi

	return $baseline_flag
}
