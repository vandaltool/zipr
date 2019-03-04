#!/bin/sh

# This script will invoke gdb on dumbledore.protected with a gdb script file 
# 

# Change to appropriate directory
curdir=$PWD
cd $1
STRATA_ANNOT_FILE=a.ncexe.annot STRATA_PC_CONFINE=1 gdb -command ../gdb.demo.script a.stratafied

cd ${curdir}
