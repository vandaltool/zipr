#!/bin/bash


elfoids=`psql -t -q -c "select elfoid from file_info"|sort|uniq`

for  i in $elfoids
do
	psql -t -q -c "\lo_unlink $i"
done


functables=`psql -t -q -c "select function_table_name from file_info"`
insntables=`psql -t -q -c "select instruction_table_name from file_info"`
addrtables=`psql -t -q -c "select address_table_name from file_info"`
grace_inpttables=`psql -t -q -c "select tablename from pg_tables where tablename like '%_input';"`
grace_covgtables=`psql -t -q -c "select tablename from pg_tables where tablename like '%_coverage';"`
othertables="variant_dependency variant_info file_info doip"

for  i in $insntables $addrtables $functables $grace_inpttables $grace_covgtables $othertables
do
	echo --------------------------------------------------------------------------
	echo -n Dropping table $i..." "
	psql -t -q -c "drop table $i cascade;"
	echo Done.
	echo --------------------------------------------------------------------------
done
