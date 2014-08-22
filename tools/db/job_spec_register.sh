#!/bin/sh -x

JOB_ID=$1
NAME=$2
VARIANT_ID=$3
STATUS=$4
SUBMITTED_TS=$5

psql -q -t -c "INSERT INTO job_spec (job_id, job_name, variant_id, status, submitted_ts) VALUES ('$JOB_ID', '$NAME', '$VARIANT_ID', '$STATUS', '$SUBMITTED_TS')"
