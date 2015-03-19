#!/bin/bash


elfoids=`psql -t -q -c "select elfoid from file_info"|sort|uniq`

for  i in $elfoids
do
	psql -t -q -c "\lo_unlink $i"
done


functables=`psql -t -q -c "select function_table_name from file_info"`
insntables=`psql -t -q -c "select instruction_table_name from file_info"`
addrtables=`psql -t -q -c "select address_table_name from file_info"`
relocstables=`psql -t -q -c "select relocs_table_name from file_info"`
grace_inpttables=`psql -t -q -c "select tablename from pg_tables where tablename like '%_input';"`
grace_covgtables=`psql -t -q -c "select tablename from pg_tables where tablename like '%_coverage';"`
othertables="variant_dependency variant_info file_info doip"

droptabs=""
dropcnt=0

for  i in $insntables $addrtables $functables $relocstables $grace_inpttables $grace_covgtables $othertables
do

	echo Dropping table $i..." "
	droptabs="$droptabs drop table $i cascade;"
	dropcnt=`expr $dropcnt + 1`
	if [ $dropcnt -gt 1000 ]; then
		echo --------------------------------------------------------------------------
		echo issuing command
		psql -t -q -c "$droptabs" || true
		echo Done.
		echo --------------------------------------------------------------------------
		dropcnt=0
		droptabs=""
	fi
done
echo dropping bonus tabs
psql -t -q -c "$droptabs" || true
