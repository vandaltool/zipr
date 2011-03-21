#!/bin/sh

#
# Argument is the directory created to store the stratafied binary
#

# produce list of candidate functions
# produce list of non-candidate functions
# produce bad asm SPRI rules for candidate functions
# produce good asm SPRI rules for candidate functions
$STRATA_REWRITE/tools/transforms/p1transform a.ncexe a.ncexe.annot

P1_DIR=p1.xform
ASPRI=aspri
BSPRI=bspri

# 
# Split out into own scripts
# Create binary SPRI files for good & bad transformation
#
cd $P1_DIR
mkdir $BSPRI
mkdir $ASPRI
for i in `ls *p1*.aspri`
do
  base=`basename $i .aspri`
  $STRATA_REWRITE/tools/spasm/spasm $i bspri/"$base".bspri  
  mv $i aspri
done

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

# remove any candidate functions not covered

CANDIDATE_FNS=$P1_DIR/a.ncexe.p1.candidates
FILTERED_OUT=$P1_DIR/a.ncexe.p1.filteredout
touch $FILTERED_OUT

KEEPS=$P1_DIR/a.ncexe.p1.keep
touch $KEEPS

while read fn;
do
  DIVERGE="no"
  echo "Evaluating candidate fn: $fn"
  BSPRI_BAD="$P1_DIR/bspri/a.ncexe.xform.p1.bad.$fn.bspri"
  echo "BSPRI_BAD=$BSPRI_BAD"

  for i in `ls $CONCOLIC/input*.json`
  do
    # run with bad SPRI transform to produce output
    input=`basename $i .json`
    echo "cmd: STRATA_SPRI_FILE=$BSPRI_BAD $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input.$fn --stderr=stderr.$input.$fn --engine=sdt ./a.stratafied $i"
    STRATA_SPRI_FILE="$BSPRI_BAD" $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input.$fn --stderr=stderr.$input.$fn --engine=sdt ./a.stratafied $i

    # if the output differs, stop right away, move to next function
    if [ ! -z replay.baseline/stdout.$input ];
    then
    echo "Diffing stdout.$input.$fn vs. replay.baseline/stdout.$input"
    diff stdout.$input.$fn replay.baseline/stdout.$input
    if [ ! $? -eq 0 ]; then
      echo "Evaluating candidate fn: $fn  BED detected divergence -- good"
      rm stdout.$input.$fn 2>/dev/null
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

    rm stdout.$input.$fn 2>/dev/null
    rm stderr.$input.$fn 2>/dev/null
  done

  if [ "$DIVERGE" = "no" ]; then
    echo "Evaluating candidate fn: $fn  BED detected no divergence -- remove fn from candidate set"
    echo $fn >> $FILTERED_OUT
  else
    echo $fn >> $KEEPS
  fi

done < $CANDIDATE_FNS


$PEASOUP_HOME/tools/p1xform.pbed.sh $P1_DIR $CANDIDATE_FNS $CONCOLIC $ASPRI $BSPRI


