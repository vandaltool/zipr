#!/bin/sh 

#
# pdb_create_program_tables <programName>
#
# WARNING: <programName> must convert non-alphanumeric characters to alphanumeric


# remove any path name

create_table()
{
	atn=$1
	ftn=$2
	itn=$3

	DB_SCRIPT=$$.script.tmp
	cat $PEASOUP_HOME/tools/db/pdb.createprogram.tbl | sed "s/#PROGNAME#/$PROGRAM_NAME/g" > $DB_SCRIPT
	psql -f $DB_SCRIPT
	rm $DB_SCRIPT
}

vid=$1

psql -q -t  -c "select address_table_name,function_table_name,instruction_table_name from file_info where variant_id='$vid'"
