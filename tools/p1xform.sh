#!/bin/sh

#
# Argument is the directory created to store the stratafied binary
#

$STRATA_REWRITE/tools/transforms/p1transform a.ncexe a.ncexe.annot

# 
# Create binary SPRI files for bad transformation
#
cd p1.xform
for i in `ls *p1*bad*.aspri`
do
base=`basename $i .aspri`
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

P1_DIR=p1.xform
# remove any candidate functions not covered
CANDIDATE_FNS=$P1_DIR/a.ncexe.p1.candidates
while read fn;
do
  echo "Evaluating candidate fn: $fn"
  BSPRI_BAD=$P1_DIR/a.ncexe.xform.p1.bad.$fn.bspri
  for i in `ls $CONCOLIC/input*.json`
  do
    # run with bad SPRI transform
    STRATA_SPRI_FILE=$BSPRI_BAD $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout --stderr --engine=sdt ./a.stratafied $i
  done

  mv grace_replay grace_replay.$fn

  # here we need to do a whole bunch of diffs to see if we've detected the bad xform
  # if none of the inputs detect the bad xform, remove from candidate set of fns to P1 transform
done < $CANDIDATE_FNS

