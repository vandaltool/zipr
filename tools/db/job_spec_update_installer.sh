#!/bin/sh -x

JOB_ID=$1
INSTALLER=$2

psql -q -t -c "UPDATE job_spec SET installer='$INSTALLER' WHERE job_id='$JOB_ID'"
