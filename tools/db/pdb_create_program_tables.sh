#!/bin/sh

#
# pdb_create_program_tables <programName>
#
# WARNING: <programName> must convert non-alphanumeric characters to alphanumeric

PROGRAM_NAME=$1

DB_SCRIPT=$$.script.tmp

PROGRAM_NAME=`echo $PROGRAM_NAME | sed "s/[\.;+\\-\ ]/_/g"`

cat $PEASOUP_HOME/tools/db/pdb.createprogram.tbl | sed "s/#PROGNAME#/$PROGRAM_NAME/g" > $DB_SCRIPT

psql -f $DB_SCRIPT

rm $DB_SCRIPT
