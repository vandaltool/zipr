#!/bin/sh -x
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
STATUS=$2
TIMESTAMP=$3

if [ $STATUS = 'pending' ]; then
	psql -q -t -c "UPDATE job_spec SET status='$STATUS', start_ts='$TIMESTAMP' WHERE job_id='$JOB_ID'"
elif [ $STATUS = 'error' ]; then
	psql -q -t -c "UPDATE job_spec SET status='$STATUS', stop_ts='$TIMESTAMP' WHERE job_id='$JOB_ID'"
else
	psql -q -t -c "UPDATE job_spec SET status='$STATUS', stop_ts='$TIMESTAMP' WHERE job_id='$JOB_ID'"
fi
