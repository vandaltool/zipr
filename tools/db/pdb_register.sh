#!/bin/sh 

#
# pdb_register <peasoup_program_name> <peasoup_program_directory> 
#
# peasoup_program_name: name of the program to register in the DB
# peasoup_program_directory: top-level directory containing all the Peasoup-related information (MEDS annotation, original binary, IDA Pro information etc...)
#

PROGRAM_NAME=$1
PROGRAM_PEASOUP_DIR=$2

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

PROGRAM_ID=`psql -q -t -c "INSERT INTO variant_info (schema_version_id,name,address_table_name,function_table_name,instruction_table_name) VALUES ('1', '$PROGRAM_NAME', '${PROGRAM_NAME}_ADDRESS', '${PROGRAM_NAME}_function', '${PROGRAM_NAME}_instruction') RETURNING variant_id;" | sed "s/^[ \t]*//"`

if [ ! $? -eq 0 ]; then
  log_error "Failed to register program"
fi

# Update original program id
psql -q -t -c "UPDATE variant_info SET orig_variant_id = '$PROGRAM_ID' WHERE variant_id = '$PROGRAM_ID';"

#============================================
# Update file_info table
#============================================


oid=`psql  -t -c "\lo_import '$FILENAME' 'original executable that was passed to ps_analyze.sh'" |cut -d" " -f2`
FILE_ID=`psql -q -t -c "INSERT INTO file_info (url, arch, hash, elfoid) VALUES ('$URL', '$ARCH', '$MD5HASH', '$oid') RETURNING file_id;" | sed "s/^[ \t]*//"`

log_message "To do: if shared libs, then need to add them to this table"

#============================================
# Update program_dependency table
#============================================
FILE_ID=`psql -q -t -c "INSERT INTO variant_dependency (variant_id, file_id) VALUES ('$PROGRAM_ID', '$FILE_ID')"`

exit $PROGRAM_ID
