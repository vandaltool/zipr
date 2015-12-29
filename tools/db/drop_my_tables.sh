#!/bin/bash
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



elfoids=`psql -t -q -c "select elfoid from file_info"|sort|uniq`

for  i in $elfoids
do
	psql -t -q -c "\lo_unlink $i"
done


functables=`psql -t -q -c "select function_table_name from file_info"`
insntables=`psql -t -q -c "select instruction_table_name from file_info"`
icfstables=`psql -t -q -c "select icfs_table_name from file_info"`
icfsmaptables=`psql -t -q -c "select icfs_map_table_name from file_info"`
addrtables=`psql -t -q -c "select address_table_name from file_info"`
relocstables=`psql -t -q -c "select relocs_table_name from file_info"`
typestables=`psql -t -q -c "select types_table_name from file_info"`
grace_inpttables=`psql -t -q -c "select tablename from pg_tables where tablename like '%_input';"`
grace_covgtables=`psql -t -q -c "select tablename from pg_tables where tablename like '%_coverage';"`
othertables="variant_dependency variant_info file_info doip"

droptabs=""
dropcnt=0

for  i in $insntables $icfstables $icfsmaptables $addrtables $functables $relocstables $typestables $grace_inpttables $grace_covgtables $othertables
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

psql -f $PEASOUP_HOME/tools/db/job.drop.tbl

echo dropping types
psql -t -q -c "DROP TYPE icfs_analysis_result;"
