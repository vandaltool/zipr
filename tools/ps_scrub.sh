#!/bin/bash -x
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
# This software was developed with SBIR funding and is subject to SBIR Data Rights, 
# as detailed below.
#
# SBIR DATA RIGHTS
#
# Contract No. __N00014-14-C-0197___W31P4Q-14-C-0086________.
# Contractor Name __Zephyr Software LLC_____________________.
# Address __2040 Tremont Road, Charlottesville, VA 22911____.
# Expiration of SBIR Data Rights Period __16-JUNE-2021______.
#
#
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
