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
FILTERED_OUT=$P1_DIR/a.ncexe.p1.filteredout

touch $FILTERED_OUT
while read fn;
do
  DIVERGE="no"
  echo "Evaluating candidate fn: $fn"
  BSPRI_BAD=$P1_DIR/bspri/a.ncexe.xform.p1.bad.$fn.bspri
  for i in `ls $CONCOLIC/input*.json`
  do
    # run with bad SPRI transform to produce output
    input=`basename $i .json`
    STRATA_SPRI_FILE=$BSPRI_BAD $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input.$fn --stderr=stderr.$input.$fn --engine=sdt ./a.stratafied $i

    # if the output differs, stop right away, move to next function
    if [ ! -z replay.baseline/stdout.$input ];
    then
    echo "Diffing stdout.$input.$fn vs. replay.baseline/stdout.$input"
    diff stdout.$input.$fn replay.baseline/stdout.$input
    if [ ! $? -eq 0 ]; then
      echo "Evaluating candidate fn: $fn  BED detected divergence -- good"

      echo "original"
      cat replay.baseline/stdout.$input
      echo "bad"
      cat stdout.$input.$fn

      rm stdout.$input.$fn
      rm stderr.$input.$fn 2>/dev/null
      
      DIVERGE="yes"
      break
    fi
    fi

#    if [ ! -z replay.baseline/stderr.$input ];
#    then
#    diff stderr.$input.$fn replay.baseline/stderr.$input
#    if [ ! $? -eq 0 ]; then
#      echo "Evaluating candidate fn: $fn  BED detected divergence -- good"
#      rm stdout.$input.$fn
#      rm stderr.$input.$fn
#      break
#    fi
#    fi

    rm stdout.$input.$fn
    rm stderr.$input.$fn
  done

  if [ "$DIVERGE" == "no" ]; then
    echo "Evaluating candidate fn: $fn  BED detected no divergence -- remove fn from candidate set"
    echo $fn >> $FILTERED_OUT
    rm p1.xform/aspri/a.ncexe.xform.p1.$fn.aspri
    rm p1.xform/aspri/a.ncexe.xform.p1.bad.$fn.aspri
  fi

done < $CANDIDATE_FNS

