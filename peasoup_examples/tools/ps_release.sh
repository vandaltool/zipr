#!/bin/bash

# Script to scrub PEASOUP directory

usage()
{
  echo "usage: $0 [ <peasoup_program> | <peasoup_dir> ]"
  exit 1
}

error()
{
  echo "$0: $1"
  echo
  usage
}

verify_peasoup_dir()
{
  ls $1 | grep ps_run > /dev/null 2>&1 
  if [ ! $? -eq 0 ]; then
    error "$1 not a valid peasoup directory"
  fi
}

verify_peasoup_app()
{
  grep ps_run $1   > /dev/null 2>&1
  if [ ! $? -eq 0 ]; then
    error "$1 not a valid peasoup program"
  fi
}


assert_files()
{
	for i in $1; do
		if [ ! -f $i ]; then 
			echo "Missing file: $i"
			exit 1
		fi
	done
}

remove_rest()
{
	keepers=$*	
	for j in `ls`; do
		found=0
		for i in $keepers; do
			if [  $i = $j ]; then 
				found=1 
			fi
		done
		if [ $found = 0 ]; then
			echo Removing $j
			if [ -d $j ]; then
				rm -Rf $j
			else
				rm -f $j
			fi
		else
			echo Keeping $j
		fi
	done
}



#
# sanity checks before scrubbing directory
#

if [ -d $1 ]; then
  peasoup_dir=$1
  verify_peasoup_dir $peasoup_dir
else
  verify_peasoup_app $1
  peasoup_dir=`grep ps_run $1 | cut -d' ' -f2`
  verify_peasoup_dir $peasoup_dir
fi

echo "Preparing directory for release: $peasoup_dir"


cd $peasoup_dir

files_to_keep="a.ncexe.annot 
a.ncexe.sigs.orig
a.stratafied 
a.irdb.fbspri.reloc
a.stratafied
a.stratafied.data_dataListFile
a.stratafied.data_hashFile
a.stratafied.data_hash.ini
a.stratafied.data_keyValueFile
a.stratafied.data_libListFile
a.stratafied.map_hashFile
a.stratafied.map_hash.ini
a.stratafied.map_keyValueFile
a.stratafied.map_libListFile
a.stratafied.term_map_hashFile
a.stratafied.term_map_hash.ini
a.stratafied.term_map_keyValueFile
diagnostics.out
libappfw.so
libstrata.so
ps_run.sh 
"

# assert that the necessary files were all created 
assert_files $files_to_keep
# remove any other files, including logs
remove_rest $files_to_keep

#
# set perms on remaining files
#

# directory has rwx for user only 
chmod 700 . a.ncexe.sigs.orig

# non-executable files are read-only 
chmod 400 a.irdb.fbspri.reloc a.ncexe.annot a.stratafied.data_dataListFile a.stratafied.data_hashFile a.stratafied.data_hash.ini a.stratafied.data_keyValueFile a.stratafied.data_libListFile a.stratafied.map_hashFile a.stratafied.map_hash.ini a.stratafied.map_keyValueFile a.stratafied.map_libListFile a.stratafied.term_map_hashFile a.stratafied.term_map_hash.ini a.stratafied.term_map_keyValueFile 

# executable files are r-x for user only 
chmod 500 libappfw.so a.stratafied libstrata.so ps_run.sh


