#!/bin/sh

#
# Argument is the directory created to store the stratafied binary
#
cd $1

$STRATA_REWRITE/tools/transforms/p1transform a.ncexe a.ncexe.annot

# 
# Create binary SPRI files for bad transformation
#
cd p1.xform
for i in `ls *p1*bad*.aspri`
do
base=`basename $i`
$STRATA_REWRITE/tools/spasm/spasm $i "$base".bspri  
done

cd -

echo Current dir:
pwd

# get output for all inputs on the normal program
CONCOLIC=concolic.files_a.stratafied_0001

for i in `ls $CONCOLIC/input*.json`
do
  $GRACE_HOME/concolic/bin/replayer --stdout --stderr --engine=ptrace ./a.ncexe $i
done

mv grace_replay grace_replay.baseline

