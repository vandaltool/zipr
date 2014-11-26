#!/bin/bash -x

# Script to scrub PEASOUP directory

usage()
{
  echo "usage: $0 [ <peasoup_dir> ]"
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

#
# sanity checks before scrubbing directory
#

if [ -d $1 ]; then
  peasoup_dir=$1
  verify_peasoup_dir $peasoup_dir
else
  usage
fi

echo "$0: scrubbing peasoup directory: $peasoup_dir"

cd $peasoup_dir

# start removing stuff
rm -fr logs/*.log
rmdir logs
rm a.irdb.aspri*
rm a.irdb.bspri
rm a.ncexe new.exe
rm *annot.full
rm *.asm
rm *.SMPobjdump
