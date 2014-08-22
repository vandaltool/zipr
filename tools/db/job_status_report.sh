#!/bin/sh -x

JOB_ID=$1
STEP=$2
STEP_NUM=$3
STATE=$4
TIMESTAMP=$5
STATUS=$6
LOGFILE=$7

#####################################################

usage()
{
  echo "report_job_status <job_id> <step_name> <step_num> [ started | completed ] <timestamp> <status> <logFile>"
}

log_error()
{
  echo "report_job_status: ERROR: $1"
  exit -1
}

log_message()
{
  echo "report_job_status: MESSAGE: $1"
}

#####################################################

if [ -z $JOB_ID ]; then
  usage
fi

if [ -z $STEP ]; then
  usage
fi

if [ -z $STEP_NUM ]; then
  usage
fi

if [ -z $TIMESTAMP ]; then
  usage
fi

if [ -z $STATUS ]; then
  usage
fi

if [ $STATE = "started" ]; then
	psql -q -t -c "INSERT INTO job_status (job_id, step, step_num, status, start_ts) VALUES ('$JOB_ID', '$STEP', '$STEP_NUM', '$STATUS', '$TIMESTAMP')"
else
	if [ -z $LOGFILE ]; then
		psql -q -t -c "UPDATE job_status SET status='$STATUS', stop_ts='$TIMESTAMP' WHERE job_id = '$JOB_ID' AND step='$STEP'"
	else
		attributes=$(grep ATTRIBUTE $LOGFILE | cut -d' ' -f3-)
		psql -q -t -c "UPDATE job_status SET status='$STATUS', stop_ts='$TIMESTAMP', log='$attributes' WHERE job_id = '$JOB_ID' AND step='$STEP'"
	fi
fi

if [ ! $? -eq 0 ]; then
  log_error "Failed to register job status"
fi

exit 0
