#!/bin/sh

#
# Assumption: we're in the top level directory created by the peasoup toolchain
#

BINARY=$1                   # subject program
BSPRI=$2                    # transformation specificiation SPRI file
INPUT_DIR=$3                # directory containing inputs
BASELINE_OUTPUT_DIR=$4      # directory containing expected outputs

echo "=========================================="
echo "Running ps_validate.sh"
echo "                BINARY: $BINARY"
echo "                 BSPRI: $BSPRI"
echo "             INPUT_DIR: $INPUT_DIR"
echo "   BASELINE_OUTPUT_DIR: $BASELINE_OUTPUT_DIR"
echo "=========================================="

for i in `ls $INPUT_DIR/input*.json`
do
  input=`basename $i .json`
  echo "ps_validate.sh: cmd: STRATA_SPRI_FILE=$BSPRI $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input.$fn --stderr=stderr.$input.$fn --engine=sdt ./a.stratafied $i"
    STRATA_SPRI_FILE="$BSPRI" "$GRACE_HOME/concolic/bin/replayer" --symbols=a.sym --stdout=stdout.$input --stderr=stderr.$input --engine=sdt ./a.stratafied $i

  if [ ! -z replay.baseline/stdout.$input ];
  then
    if [ ! $? -eq 0 ]; then
      echo "ps_validate.sh: divergence detected for input $i (stdout)"

      echo "Baseline file (stdout):"
      cat replay.baseline/stdout.$input

      echo "Output stdout (stdout):$input:"
      cat stdout.$input

      exit 1
    fi
  fi

  if [ ! -z replay.baseline/stderr.$input ];
  then
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

exit 0
