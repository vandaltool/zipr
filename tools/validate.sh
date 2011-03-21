#!/bin/sh
#
# Validate transformed program
#
#   $1 stratafied binary to validate
#   $2 binary SPRI file
#   $3 directory of inputs
#   $4 directory of baseline outputs
#

echo "=========================================="
echo "Running program validator"
echo "=========================================="

STRATAFIED_BINARY=$1   # stratafied binary
BSPRI_FILE=$2          # binary spri file
INPUT_DIR=$3           # directory with inputs
OUTPUT_DIR=$4          # directory of baseline output

for i in `ls $INPUT_DIR/input*.json`
do
  input=`basename $i .json`
  echo "Validating on input $input"
  STRATA_SPRI_FILE=$BSPRI_FILE $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input.$fn --stderr=stderr.$input.$fn --engine=sdt $STRATAFIED_BINARY $i

  if [ ! -z replay.baseline/stdout.$input ];
  then
    diff stdout.$input.$fn replay.baseline/stdout.$input
    if [ ! $? -eq 0 ]; then
      echo "ERROR -- output divergence detected on input: $i"
      exit 1
    fi

    rm stdout.$input.$fn 2>/dev/null
    rm stderr.$input.$fn 2>/dev/null
  fi
done

