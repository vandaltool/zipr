#!/bin/bash
#
# Copyright (c) 2014 - Zephyr Software LLC
#
# This file may be used and modified for non-commercial purposes as long as
# all copyright, permission, and nonwarranty notices are preserved.
# Redistribution is prohibited without prior written consent from Zephyr
# Software.
#
# Please contact the authors for restrictions applying to commercial use.
#
# THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# Author: Zephyr Software
# e-mail: jwd@zephyr-software.com
# URL   : http://www.zephyr-software.com/
#
# This software was developed with SBIR funding and is subject to SBIR Data Rights, 
# as detailed below.
#
# SBIR DATA RIGHTS
#
# Contract No. __N00014-14-C-0197___W31P4Q-14-C-0086________.
# Contractor Name __Zephyr Software LLC_____________________.
# Address __2040 Tremont Road, Charlottesville, VA 22911____.
# Expiration of SBIR Data Rights Period __16-JUNE-2021______.
#


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
