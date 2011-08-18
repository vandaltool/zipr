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

REPLAYER_TIMEOUT=120         # timeout value for the replayer -- for now 120 seconds per input

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

for i in `ls $INPUT_DIR/input*.json`
do
  echo ""
  input=`basename $i .json`
  input_number=`echo $input | sed "s/input_//"`

  # make sure the output files exist
  touch stdout.$input
  touch stderr.$input

  echo "ps_validate.sh: cmd: STRATA_SPRI_FILE=$BSPRI timeout $REPLAYER_TIMEOUT $GRACE_HOME/concolic/bin/replayer --symbols=a.sym --stdout=stdout.$input --stderr=stderr.$input --engine=sdt ./a.stratafied $i"
  STRATA_SPRI_FILE="$BSPRI" "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=a.sym --stdout=stdout.$input --stderr=stderr.$input --engine=sdt $STRATAFIED_BINARY $i
#    STRATA_SPRI_FILE="$BSPRI" timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=120 --symbols=a.sym --stdout=stdout.$input --stderr=stderr.$input --engine=sdt $STRATAFIED_BINARY $i || exit 2

  BASELINE_OUTPUT_STDOUT=$BASELINE_OUTPUT_DIR/run_$input_number/stdout
  BASELINE_OUTPUT_STDERR=$BASELINE_OUTPUT_DIR/run_$input_number/stderr

  echo "$BASELINE_OUTPUT_STDOUT"
  echo "$BASELINE_OUTPUT_STDERR"
  
  if [ -f $BASELINE_OUTPUT_STDOUT ];
  then
    diff stdout.$input $BASELINE_OUTPUT_STDOUT
    if [ ! $? -eq 0 ]; then
      echo "ps_validate.sh: divergence detected for input: $i (stdout)"

      echo "Baseline file (stdout):"
      cat $BASELINE_OUTPUT_STDOUT

      echo "Output stdout (stdout):$input:"
      cat stdout.$input

      exit 1
    fi
  fi

  if [ -f $BASELINE_OUTPUT_STDERR ];
  then
    diff stderr.$input $BASELINE_OUTPUT_STDERR
    if [ ! $? -eq 0 ]; then
      echo "ps_validate.sh: divergence detected for input: $i (stderr)"

      echo "Baseline file (stderr):"
      cat $BASELINE_OUTPUT_STDERR

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
