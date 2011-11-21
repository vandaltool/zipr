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

rm -fr replay 2>/dev/null
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

for i in `ls $INPUT_DIR/input*.json`
do

 #Ben Modification: run replayer from top level to keep the sandbox layout the same as produced by the concolic test engine. This makes output comparison easier
  echo "Testing input spec: $i"
  input=`basename $i .json`
  input_number=`echo $input | sed "s/input_//"`


  abridged_number=`echo $input_number | sed 's/0*\(.*\)/\1/'`
  #if there is no exit code for the input number, skip for now.
  if [ ! -f "$INPUT_DIR/exit_code.run_$abridged_number.log" ]; then
      echo "ps_validate.sh: No baseline data for input $input_number, missing exit status"
      continue;
  fi
  echo "Exit status baseline file: $INPUT_DIR/exit_code.run_$abridged_number.log"
  #if baseline exited with 139, ignore input
  grep 139 $INPUT_DIR/exit_code.run_$abridged_number.log
  if [ $? -eq 0 ]; then
      echo "Baseline exit status was 139, ignoring input"
      continue
  fi

  mkdir replay/$input_number 2>/dev/null
# make sure the output files exist
  touch replay/$input_number/stdout.$input
  touch replay/$input_number/stderr.$input

  #cleanup any previous runs
  rm -rf grace_replay
  rm -f stdout.* stderr.* exit_status

  echo "ps_validate.sh: cmd: STRATA_SPRI_FILE=$BSPRI timeout $REPLAYER_TIMEOUT $GRACE_HOME/concolic/bin/replayer --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt $STRATAFIED_BINARY $i"
  STRATA_SPRI_FILE="$BSPRI" timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt $STRATAFIED_BINARY $i || exit 2

  mv stderr.$input replay/$input_number/stderr.$input
  mv stdout.$input replay/$input_number/stdout.$input
  mv exit_status replay/$input_number/exit_status

#first verify the exit status 

  diff replay/$input_number/exit_status $INPUT_DIR/exit_code.run_$abridged_number.log

  if [ ! $? -eq 0 ]; then
      echo "ps_validate.sh: divergence detected for input: $i (exit status)"
      echo "expected: "
      cat $INPUT_DIR/exit_code.run_$abridged_number.log
      echo "observed: "
      cat replay/$input_number/exit_status
      exit 1
  fi

  #next check program produced output files, if any exist

  if [ -d "grace_replay" ];then
      echo "ps_validate.sh: Discovered output files, validating contents"
      mv  grace_replay/ replay/$input_number/
      cp $BASELINE_OUTPUT_DIR/run_$input_number/* replay/$input_number/grace_replay/replay_0001/
      diff -r $BASELINE_OUTPUT_DIR/run_$input_number/ replay/$input_number/grace_replay/replay_0001 >diff_tmp
      if [ ! $? -eq 0 ]; then
	  echo "ps_validate.sh: divergence detected for input: $i (output files)"
	  echo "Diff output:"
	  cat diff_tmp
	  rm diff_tmp
	  exit 1
      fi
      rm diff_tmp
  fi

  cd replay/$input_number

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

  cd ../..

done

echo "ps_validate.sh: All inputs validated"
exit 0
