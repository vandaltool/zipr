#!/bin/sh

#
# Argument is the directory created to store the stratafied binary
#

# produce list of candidate functions
# produce list of non-candidate functions
# produce bad asm SPRI rules for candidate functions
# produce good asm SPRI rules for candidate functions
$STRATA_REWRITE/tools/transforms/p1transform a.ncexe a.ncexe.annot

# 
# Create binary SPRI files for bad transformation
#
cd p1.xform
mkdir bspri
mkdir aspri
for i in `ls *p1*bad*.aspri`
do
  base=`basename $i .aspri`
  $STRATA_REWRITE/tools/spasm/spasm $i bspri/"$base".bspri  
  mv $i aspri
done

# Also move the good aspri files
mv *.aspri aspri

cd -

echo Current dir:
pwd

# get output for all inputs on the normal program
echo "Running replayer to get baseline outputs"
CONCOLIC=concolic.files_a.stratafied_0001

mkdir replay.baseline
for i in `ls $CONCOLIC/input*.json`
do
  # format input file is:  input_0001.json
  input=`basename $i .json`
  $GRACE_HOME/concolic/bin/replayer --stdout=replay.baseline/stdout.$input --stderr=replay.baseline/stderr.$input --engine=ptrace ./a.ncexe $i
done

P1_DIR=p1.xform
# remove any candidate functions not covered
CANDIDATE_FNS=$P1_DIR/a.ncexe.p1.candidates
while read fn;
do
  echo "Evaluating candidate fn: $fn"
  BSPRI_BAD=$P1_DIR/bspri/a.ncexe.xform.p1.bad.$fn.bspri
  for i in `ls $CONCOLIC/input*.json`
  do
    # run with bad SPRI transform to produce output
    input=`basename $i .json`
    STRATA_SPRI_FILE=$BSPRI_BAD $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input.$fn --stderr=stderr.$input.$fn --engine=sdt ./a.stratafied $i

    # if the output differs, stop right away, move to next function
    diff stdout.$input.$fn replay.baseline/stdout.$input
    if [ ! $? -eq 0 ]; then
      echo "Evaluating candidate fn: $fn  TSET=0 Remove from candidate set (STDOUT differ)"
      rm stdout.$input.$fn
      rm stderr.$input.$fn
      rm p1.xform/aspri/a.ncexe.xform.p1.$fn.aspri
      rm p1.xform/bspri/a.ncexe.xform.p1.$fn.bspri
      break
    fi

    diff stderr.$input.$fn replay.baseline/stderr.$input
    if [ ! $? -eq 0 ]; then
      echo "Evaluating candidate fn: $fn  TSET=0 Remove from candidate set (STDERR differ)"
      rm stdout.$input.$fn
      rm stderr.$input.$fn
      rm p1.xform/aspri/a.ncexe.xform.p1.$fn.aspri
      rm p1.xform/bspri/a.ncexe.xform.p1.$fn.bspri
      break
    fi

    rm stdout.$input.$fn
    rm stderr.$input.$fn
  done

  # here we need to do a whole bunch of diffs to see if we've detected the bad xform
  # if none of the inputs detect the bad xform, remove from candidate set of fns to P1 transform

done < $CANDIDATE_FNS

