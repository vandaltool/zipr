#!/bin/sh

# $1 is the top-level directory
# $2 is the binary (non-stratafied)
# $3 is the directory that contains the inputs


TOP_DIR=$1
BINARY=`basename $2`
INPUT_DIR=$3

BASELINE_OUTPUT_DIR=replay.baseline

echo "=========================================================================="
echo "Running replayer to get baseline outputs on binary: $TOP_DIR/$BINARY"
echo "                                   Input directory: $INPUT_DIR"
echo "                                  Output directory: $BASELINE_OUTPUT_DIR"
echo "=========================================================================="

cd $TOP_DIR

mkdir $BASELINE_OUTPUT_DIR

for i in `ls $INPUT_DIR/input*.json`
do
  # format input file is:  input_0001.json
  input=`basename $i .json`
  $GRACE_HOME/concolic/bin/replayer --stdout=$BASELINE_OUTPUT_DIR/stdout.$input --stderr=$BASELINE_OUTPUT_DIR/stderr.$input --engine=ptrace ./$BINARY $i
done

cd -
