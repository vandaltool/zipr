#!/bin/sh

#
# Assumption: we're in the top level directory created by the peasoup toolchain
#

STRATAFIED_BINARY=$1        # stratafied subject program
BSPRI=$2                    # transformation specificiation SPRI file
INPUT_DIR=$3                # directory containing inputs
BASELINE_OUTPUT_DIR=$4      # directory containing expected outputs

echo "=========================================="
echo "Running ps_validate.sh"
echo "                STRATAFIED_BINARY: $STRATAFIED_BINARY"
echo "                 BSPRI: $BSPRI"
echo "             INPUT_DIR: $INPUT_DIR"
echo "   BASELINE_OUTPUT_DIR: $BASELINE_OUTPUT_DIR"
echo "=========================================="

for i in `ls $INPUT_DIR/input*.json`
do
  echo ""
  input=`basename $i .json`
  echo "ps_validate.sh: cmd: STRATA_SPRI_FILE=$BSPRI $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input --stderr=stderr.$input --engine=sdt ./a.stratafied $i"
    STRATA_SPRI_FILE="$BSPRI" "$GRACE_HOME/concolic/bin/replayer" --symbols=a.sym --stdout=stdout.$input --stderr=stderr.$input --engine=sdt $STRATAFIED_BINARY $i

  if [ -f replay.baseline/stdout.$input ];
  then
    diff replay.baseline/stdout.$input stdout.$input
    if [ ! $? -eq 0 ]; then
      echo "ps_validate.sh: divergence detected for input $i (stdout)"

      echo "Baseline file (stdout):"
      cat replay.baseline/stdout.$input

      echo "Output stdout (stdout):$input:"
      cat stdout.$input

      exit 1
    fi
  fi

  if [ -f replay.baseline/stderr.$input ];
  then
    diff replay.baseline/stderr.$input stderr.$input
    if [ ! $? -eq 0 ]; then
      echo "ps_validate.sh: divergence detected for input $i (stderr)"

      echo "Baseline file (stderr):"
      cat replay.baseline/stderr.$input

      echo "Output stderr (stderr):$input:"
      cat stderr.$input

      exit 1
    fi
  fi

  echo "ps_validate.sh: input $i validates"

  # remove tmp files
  rm stdout.$input 2>/dev/null
  rm stderr.$input 2>/dev/null
done

echo "ps_validate.sh: All inputs validated"
exit 0
