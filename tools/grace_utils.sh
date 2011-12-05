#!/bin/bash

#
# Utilities to assess state of Grace-produced inputs & outputs
#

#
# Did Grace attempt to symbolically execute anything?
#
# Returns number of inputs symbolically executed
#
get_grace_number_inputs_executed()
{
	CONCOLIC_DIR=$1

	if [ -z $CONCOLIC_DIR ]; then
		echo "check_grace_baseline(): no concolic input/output directory specified"
		return 0
	fi

	if [ ! -d $CONCOLIC_DIR ]; then
		echo "check_grace_baseline(): specified concolic input/output directory does not exist"
		return 0
	fi

	baseline_flag=`ls $CONCOLIC_DIR/sandboxed-files | grep run_ | wc -l`
	return $baseline_flag
}
