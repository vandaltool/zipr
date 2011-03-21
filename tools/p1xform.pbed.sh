#!/bin/sh
#
# $1 is a file containing list of functions to test
# $2 specifies directory with the inputs found by GrACE
# $3 specifies directory containing all the assembly SPRI rules
# $4 specifies directory containing all the binary SPRI rules
#
# Output: 
#   Binary spri file specification for P1 xform 
#


P1DIR=$1           # top-level P1 xform directory 
FNS=$2             # file containing name of functions to evaluate
CONCOLIC=$3        # directory with inputs
ASPRI=$4           # directory with assembly SPRI rules
BSPRI=$5           # directory with binary SPRI rules

echo "=========================================="
echo "Running p1xform.pbed.sh"
echo "P1DIR=$1"
echo "FNS=$2" 
echo "CONCOLIC=$3"
echo "ASPRI=$4"
echo "BSPRI=$5"
echo "=========================================="

P1_GOOD_FILE=$P1DIR/a.ncexe.p1.final
touch $P1_GOOD_FILE

while read fn;
do
  echo "Checking for divergence on function $fn"

  DIVERGE="no"
  BSPRI_GOOD="$P1DIR/$BSPRI/a.ncexe.xform.p1.$fn.bspri"

  for i in `ls $CONCOLIC/input*.json`
  do
    echo "Doing BED on function $fn"

    input=`basename $i .json`
    echo "p1xform.pbed.sh: cmd: STRATA_SPRI_FILE=$BSPRI_GOOD $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input.$fn --stderr=stderr.$input.$fn --engine=sdt ./a.stratafied $i"
    STRATA_SPRI_FILE="$BSPRI_GOOD" $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input.$fn --stderr=stderr.$input.$fn --engine=sdt ./a.stratafied $i

    # if the output differs, stop right away, move to next function
    if [ ! -z replay.baseline/stdout.$input ];
    then
    echo "Diffing stdout.$input.$fn vs. replay.baseline/stdout.$input"
    diff stdout.$input.$fn replay.baseline/stdout.$input
    if [ ! $? -eq 0 ]; then
      echo "BED: divergence detected for fn: $fn on input $i"
      echo "Baseline file:"
      cat replay.baseline/stdout.$input
      echo "Output stdout:$input.$fn:"
      cat stdout.$input.$fn

      rm stdout.$input.$fn 2>/dev/null
      rm stderr.$input.$fn 2>/dev/null
      DIVERGE="yes"
      break
    fi
    fi
  done

  # remove tmp files
  rm stdout.$input.$fn 2>/dev/null
  rm stderr.$input.$fn 2>/dev/null

  if [ "$DIVERGE" = "no" ]; then
    echo $fn >> $P1_GOOD_FILE
  fi

done < $FNS

