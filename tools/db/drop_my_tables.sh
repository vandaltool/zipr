#!/bin/bash


functables=`psql -t -q -c "select function_table_name from variant_info"`
insntables=`psql -t -q -c "select instruction_table_name from variant_info"`
addrtables=`psql -t -q -c "select address_table_name from variant_info"`
othertables="variant_dependency variant_info file_info doip"

for  i in $insntables $addrtables $functables $othertables
do
	echo --------------------------------------------------------------------------
	echo -n Dropping table $i..." "
	psql -t -q -c "drop table $i cascade;"
	echo Done.
	echo --------------------------------------------------------------------------
done
