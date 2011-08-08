#!/bin/bash


elfoids=`psql -t -q -c "select elfoid from file_info"`

for  i in $elfoids
do
	psql -t -q -c "\lo_unlink $i"
done


functables=`psql -t -q -c "select function_table_name from variant_info"`
insntables=`psql -t -q -c "select instruction_table_name from variant_info"`
addrtables=`psql -t -q -c "select address_table_name from variant_info"`
othertables="variant_dependency variant_info file_info doip"

for  i in $insntables $addrtables $functables $othertables
do
	echo -n Dropping table $i..." "
	psql -t -q -c "drop table $i cascade;"
	echo Done.
done
