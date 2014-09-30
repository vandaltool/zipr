#!/bin/sh

psql -f $PEASOUP_HOME/tools/db/pdb.create.tbl
psql -f $PEASOUP_HOME/tools/db/job.create.tbl
