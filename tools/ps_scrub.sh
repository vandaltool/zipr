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
  ls $1 | grep ps_run
  if [ ! $? -eq 0 ]; then
    error "$1 not a valid peasoup directory"
  fi
}

verify_peasoup_app()
{
  grep ps_run $1 
  if [ ! $? -eq 0 ]; then
    error "$1 not a valid peasoup program"
  fi
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

echo "$0: scrubbing peasoup directory: $peasoup_dir"

peasoup_dir_basename=`basename $1`

cd "$peasoup_dir/.."

# make sure we have permission to write by creating a temporary directory
TMP=$peasoup_dir_basename.$$
mkdir "$TMP"

if [ ! $? -eq 0 ]; then
  error "Could not create temporary directory: $TMP"
fi

cp $peasoup_dir/a.irdb.bspri $TMP
cp $peasoup_dir/a.ncexe.annot $TMP
cp $peasoup_dir/a.stratafied $TMP
cp $peasoup_dir/ps_run.sh $TMP

# mv directory
junk_it="$peasoup_dir_basename.about_to_be_removed"
mv "$peasoup_dir_basename" "$junk_it"
mv "$TMP" "$peasoup_dir"

if [ -d "$junk_it" ]; then
  rm -fr "$junk_it"
fi

