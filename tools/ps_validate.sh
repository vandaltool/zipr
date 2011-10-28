#!/bin/sh

#
# Assumption: we're in the top level directory created by the peasoup toolchain
#
# Validate SPRI transform against a suite of input/output pairs
#
# TODO: validate against all files created by the original run, not just stdout, stderr
#

# Inputs
STRATAFIED_BINARY=$1        # stratafied subject program (a.stratafied)
BSPRI=$2                    # transformation specification SPRI file (some bspri file)
INPUT_DIR=$3                # directory containing inputs (.../concolic.files_a.stratafied_0001)

BASELINE_OUTPUT_DIR=$INPUT_DIR/sandboxed-files

REPLAYER_TIMEOUT=120        # timeout value for when replaying input -- for now 120 seconds per input

TOP_LEVEL=`pwd`

mkdir replay 2>/dev/null

echo "=========================================="
echo "Running ps_validate.sh"
echo "                STRATAFIED_BINARY: $STRATAFIED_BINARY"
echo "                 BSPRI: $BSPRI"
echo "             INPUT_DIR: $INPUT_DIR"
echo "   BASELINE_OUTPUT_DIR: $BASELINE_OUTPUT_DIR"
echo "=========================================="

#
# name of files describing inputs is of the form: input_0001.json, input_0002.json, ...
#

echo "ps_validate.sh: BED: warning: @todo: need to handle files other than stdout, stderr"

cd replay

for i in `ls $INPUT_DIR/input*.json`
do
  echo "Testing input spec: $i"
  input=`basename $i .json`
  input_number=`echo $input | sed "s/input_//"`

  mkdir $input_number 2>/dev/null
  cd $input_number

  # make sure the output files exist
  touch stdout.$input
  touch stderr.$input

  echo "ps_validate.sh: cmd: STRATA_SPRI_FILE=$BSPRI timeout $REPLAYER_TIMEOUT $GRACE_HOME/concolic/bin/replayer --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --engine=sdt $STRATAFIED_BINARY $i"
  STRATA_SPRI_FILE="$BSPRI" timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --engine=sdt $STRATAFIED_BINARY $i || exit 2

  BASELINE_OUTPUT_STDOUT=$BASELINE_OUTPUT_DIR/run_$input_number/stdout
  BASELINE_OUTPUT_STDERR=$BASELINE_OUTPUT_DIR/run_$input_number/stderr

  echo "$BASELINE_OUTPUT_STDOUT"
  echo "$BASELINE_OUTPUT_STDERR"
  
  if [ -f $BASELINE_OUTPUT_STDOUT ];
  then
    echo ""
	pwd
	echo "diffing stdout.$input $BASELINE_OUTPUT_STDOUT"
    diff stdout.$input $BASELINE_OUTPUT_STDOUT
    if [ ! $? -eq 0 ]; then
      echo "ps_validate.sh: divergence detected for input: $i (stdout)"

      echo "--- Baseline file (stdout) ---"
      cat $BASELINE_OUTPUT_STDOUT

      echo "--- Output stdout (stdout) for input: $input ---"
      cat stdout.$input

      exit 1
    fi
  fi

  if [ -f $BASELINE_OUTPUT_STDERR ];
  then
    diff stderr.$input $BASELINE_OUTPUT_STDERR
    if [ ! $? -eq 0 ]; then
      echo "ps_validate.sh: divergence detected for input: $i (stderr)"

      echo "--- Baseline file (stderr) ---"
      cat $BASELINE_OUTPUT_STDERR

      echo "--- Output stderr (stderr) for input: $input ---"
      cat stderr.$input

      exit 1
    fi
  fi

  echo "ps_validate.sh: input $i validates"

  # remove tmp files
#  rm stdout.$input 2>/dev/null
#  rm stderr.$input 2>/dev/null

  cd -

done

cd -

echo "ps_validate.sh: All inputs validated"
exit 0
