#!/bin/sh  -x

#
# pdb_create_program_tables <atn> <ftn> <itn> <rtn> file
#


# remove any path name

atn=$1
ftn=$2
itn=$3
rtn=$4
file=$5

echo Creating tables $atn, $ftn, $itn, and $rtn.

DB_SCRIPT=$file
cat $PEASOUP_HOME/tools/db/pdb.createprogram.tbl |  \
                sed "s/#ATN#/$atn/g" | \
                sed "s/#FTN#/$ftn/g" | \
                sed "s/#ITN#/$itn/g" | \
                sed "s/#RTN#/$rtn/g"  \
                > $DB_SCRIPT

