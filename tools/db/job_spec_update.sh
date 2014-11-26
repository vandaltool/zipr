#!/bin/sh -x

JOB_ID=$1
STATUS=$2
TIMESTAMP=$3

if [ $STATUS = 'pending' ]; then
	psql -q -t -c "UPDATE job_spec SET status='$STATUS', start_ts='$TIMESTAMP' WHERE job_id='$JOB_ID'"
elif [ $STATUS = 'error' ]; then
	psql -q -t -c "UPDATE job_spec SET status='$STATUS', stop_ts='$TIMESTAMP' WHERE job_id='$JOB_ID'"
else
	psql -q -t -c "UPDATE job_spec SET status='$STATUS', stop_ts='$TIMESTAMP' WHERE job_id='$JOB_ID'"
fi
