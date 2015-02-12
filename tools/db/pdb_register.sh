#!/bin/sh -x
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


#
# pdb_register <peasoup_program_name> <peasoup_program_directory> 
#
# peasoup_program_name: name of the program to register in the DB
# peasoup_program_directory: top-level directory containing all the Peasoup-related information (MEDS annotation, original binary, IDA Pro information etc...)
#

PROGRAM_NAME=$1
PROGRAM_PEASOUP_DIR=$2
VARIANT_ID_OUTPUT=$3

#####################################################

usage()
{
  echo "pdb_register <peasoup_program_name> <peasoup_program_directory> "
}

log_error()
{
  echo "pdb_register: ERROR: $1"
  exit -1
}

log_message()
{
  echo "pdb_register: MESSAGE: $1"
}

#####################################################

if [ -z $PROGRAM_NAME ]; then
  usage
fi

if [ ! -d $PROGRAM_PEASOUP_DIR ]; then
  log_error "Program peasoup directory: $PROGRAM_PEASOUP_DIR was not found"
fi

# Go inside the top-level directory
cd $2

HOSTNAME=`hostname`
FILENAME=`pwd`/a.ncexe
URL="file://$HOSTNAME$FILENAME"
ARCH=`uname -m`

if [ ! -f $FILENAME ]; then
  log_error "Could not find ELF file at: $FILENAME"
fi

MD5HASH=`md5sum $FILENAME | cut -f1 -d' '`

#============================================
# Update variant_info table
#============================================

# -q: quiet mode
# -t: tuple only
# -c: run command


PROGRAM_ID=`psql -q -t -c "INSERT INTO variant_info (schema_version_id,name) VALUES ('2', '$PROGRAM_NAME') RETURNING variant_id;" | sed "s/^[ \t]*//"`

if [ ! $? -eq 0 ]; then
  log_error "Failed to register program"
fi

# Update original program id
psql -q -t -c "UPDATE variant_info SET orig_variant_id = '$PROGRAM_ID' WHERE variant_id = '$PROGRAM_ID';"



#============================================
# create the tables for this file
#============================================
create_table()
{
	$PEASOUP_HOME/tools/db/pdb_create_program_tables.sh $1 $2 $3 $4 $5 db.tmp.$$
	psql -q -t -c "`cat db.tmp.$$`"
}

#============================================
# Update file_info table
#============================================

update_file_info()
{
	pn=$1
	url=$2
	arch=$3
	md5=$4
	fn=$5
	pid=$6
	comment=$7

	echo Adding $fn to IRDB.

	pn=pn_${pn}_$pid
	pn=`echo $pn | sed "s/[^a-zA-Z0-9]/_/g"`
	
	oid=`psql  -t -c "\lo_import '$fn' '$comment'" |cut -d" " -f2`
	FILE_ID=`psql -q -t -c "INSERT INTO file_info (url, arch, hash, elfoid) VALUES ('$url', '$arch', '$md5', '$oid') RETURNING file_id;" | sed "s/^[ \t]*//"`

	# the sing the program name is a problem if the prog. name is over 40 chars.
	# just use a pid and fid.
	pn=table_${pid}_${FILE_ID}

	# Update original file id
	psql -q -t -c "UPDATE file_info SET orig_file_id = '$FILE_ID', address_table_name = '${pn}_address', function_table_name = '${pn}_function', instruction_table_name = '${pn}_instruction', relocs_table_name = '${pn}_relocs', types_table_name = '${pn}_types' WHERE file_id = '$FILE_ID';" || exit 1

	# update the variant dependency table
	psql -q -t -c "INSERT INTO variant_dependency (variant_id, file_id) VALUES ('$pid', '$FILE_ID')" || exit 1

	create_table ${pn}_address ${pn}_function ${pn}_instruction ${pn}_relocs ${pn}_types

	echo Importing $fn.annot into IRDB via meds2pdb 
	$SECURITY_TRANSFORMS_HOME/tools/meds2pdb/meds2pdb ${fn}.annot ${fn}.infoannot $FILE_ID ${pn}_function ${pn}_instruction ${pn}_address ${pn}_types  $fn ${fn}.STARSxrefs || exit 1
}





#============================================
# insert a.ncexe into the table
#============================================
update_file_info $PROGRAM_NAME $URL $ARCH $MD5HASH $FILENAME $PROGRAM_ID "original executable that was passed to ps_analyze.sh"

#============================================
# insert each of the shared libraries into the table
#============================================
for i in `cat shared_libs`; do
	echo registering $i	
	myname=$i
	myfn=`pwd`/shared_objects/$i
	myurl="file://`hostname`$myfn"
	mymd5=`md5sum $myfn | cut -f1 -d' '`

	update_file_info $myname $myurl $ARCH $mymd5 $myfn $PROGRAM_ID ".so for a.ncexe, $i"
done


#============================================
# write out the program id so we have it for later.
#============================================
rm $VARIANT_ID_OUTPUT 2>/dev/null
echo $PROGRAM_ID > $VARIANT_ID_OUTPUT

exit 0
