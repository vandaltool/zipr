#!/bin/sh 

psql -f $PEASOUP_HOME/tools/db/pdb.drop.tbl
psql -f $PEASOUP_HOME/tools/db/job.drop.tbl
