#!/bin/sh

#
# Run this script from top-level directory created by the peasoup script
#

CURRENT_DIR=`pwd`

P1_DIR=p1.xform

mkdir $P1_DIR

ASPRI_DIR=$P1_DIR/aspri
BSPRI_DIR=$P1_DIR/bspri

echo ""
echo "=========================================="
echo "p1xform.sh script started in $CURRENT_DIR"
echo "P1 transform directory: $P1_DIR"
echo "=========================================="

$PEASOUP_HOME/tools/p1xform.genspri.sh $P1_DIR a.ncexe a.ncexe.annot > $P1_DIR/genspri.out 2> $P1_DIR/genspri.err

$PEASOUP_HOME/tools/generate_io_baseline.sh $CURRENT_DIR a.ncexe concolic.files_a.stratafied_0001 > gen_baseline.out 2> gen_baseline.err

#
# remove any candidate functions not covered
# this will go away once GrACE gives us the instruction coverage information
#
CONCOLIC=concolic.files_a.stratafied_0001

CANDIDATE_FNS=$P1_DIR/p1.candidates
FILTERED_OUT=$P1_DIR/p1.fn_coverage.filtered_out
touch $FILTERED_OUT

KEEPS=$P1_DIR/p1.keep
touch $KEEPS

while read fn;
do
  DIVERGE="no"
  echo "Evaluating candidate fn: $fn"
  BSPRI_BAD="$P1_DIR/bspri/p1.bad.$fn.bspri"
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

echo "====================================================="
echo "1st pass: DONE EVALUATING CANDIDATE FUNCTIONS"
echo "====================================================="

echo "====================================================="
echo "Run BED"
echo "====================================================="
cd $CURRENT_DIR
$PEASOUP_HOME/tools/p1xform.pbed.sh $P1_DIR $KEEPS $CONCOLIC $BSPRI_DIR

echo "====================================================="
echo "Produce final transformed binary"
echo "====================================================="
cd $CURRENT_DIR
$PEASOUP_HOME/tools/p1xform.doxform.sh $P1_DIR/p1.final $P1_DIR $ASPRI_DIR


