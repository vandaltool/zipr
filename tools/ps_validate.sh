#!/bin/sh

#
# Assumption: we're in the top level directory created by the peasoup toolchain
#
# Validate SPRI transform against a suite of input/output pairs
#
# TODO: I assume that pwd is the peasoup directory containing the stratafied
# and original. Perhaps this should be passed in in the future. Also
# this script will exit in some cases without cd'ing back to the original
# directory it started in.
#

# Inputs
STRATAFIED_BINARY=$1        # stratafied subject program (a.stratafied)
BSPRI=$2                    # transformation specification SPRI file (some bspri file)
INPUT_DIR=$3                # directory containing inputs (.../concolic.files_a.stratafied_0001)

BASELINE_OUTPUT_DIR=$INPUT_DIR/sandboxed-files

REPLAYER_TIMEOUT=120        # timeout value for when replaying input -- for now 120 seconds per input

ORIG_PROG=a.ncexe
baseline_cnt=0
TOP_LEVEL=`pwd`

EMPTY_JSON=$PEASOUP_HOME/tools/empty.json

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
      echo "Baseline exit status was 139 for input $i, ignoring input"
      continue
  fi
  #if baseline exited with 134, ignore input
  grep 134 $INPUT_DIR/exit_code.run_$abridged_number.log
  if [ $? -eq 0 ]; then
      echo "Baseline exit status was 134 for input $i, ignoring input"
      continue
  fi

  #at this point we know we have baseline data to compare against
  #we assume that if exit status was produced, baseline information is available
  baseline_cnt=`expr $baseline_cnt + 1`

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
      diff -r -B -I stratafied $BASELINE_OUTPUT_DIR/run_$input_number/ replay/$input_number/grace_replay/replay_0001 >diff_tmp
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
    diff -B -I stratafied stdout.$input $BASELINE_OUTPUT_STDOUT
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
    diff -B -I stratafied stderr.$input $BASELINE_OUTPUT_STDERR
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

#if no baseline run was found, run the original program with no input
#If the original program doesn't segfault (139), compare the original run
#exit status against the transformed program exist status
if [ $baseline_cnt -eq 0 ];then

    echo "ps_validate.sh: No valid baseline to compare against, performing simple no args sanity check instead"

#TODO: need to remove EMPTY_JSON, used only until duc provides empty functionality to grace.

#check to see if the sym file exists, if not create it.
    if [ ! -e $TOP_LEVEL/a.sym ]; then
	$GRACE_HOME/concolic/src/util/linux/objdump_to_grace $STRATAFIED_BINARY
    fi

#only generate the original program exit status for no input if it doesn't
#already exist
    if [ ! -e $TOP_LEVEL/orig_status ]; then
	timeout $REPLAYER_TIMEOUT ./$ORIG_PROG &>/dev/null
	orig_status=$?
	echo "$orig_status" >$TOP_LEVEL/orig_status
    else
	orig_status=`cat $TOP_LEVEL/orig_status`
    fi

    if [ $orig_status -ne 139 ];then
	echo "STRATA_SPRI_FILE="$BSPRI" timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt $STRATAFIED_BINARY $EMPTY_JSON || exit 2"
	STRATA_SPRI_FILE="$BSPRI" timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt $STRATAFIED_BINARY $EMPTY_JSON || exit 2
#	echo "STRATA_SPRI_FILE=$BSPRI timeout $REPLAYER_TIMEOUT  $TOP_LEVEL/a.stratafied" 
#	STRATA_SPRI_FILE=$BSPRI timeout $REPLAYER_TIMEOUT  $TOP_LEVEL/a.stratafied 
##	timeout $REPLAYER_TIMEOUT ./ps_run.sh $TOP_LEVEL
#	trans_status=$?

#	if [ $orig_status -ne $trans_status ]; then
#	    echo "Status values do not equal, orig_status $orig_status, trans_status $trans_status"
#	    exit 1
	    #else fall through, exit 0, 
#	fi

	#just in case there is a replayer failure, create an exit status file
	touch exit_status
	echo "Subject exited with status $orig_status" >tmp
	cat exit_status | grep "Subject exited with status" >tmp2
	diff tmp tmp2
	diff_status=$?
	rm -f tmp tmp2
	if [ $diff_status -ne 0 ]; then
	    echo "ps_validate.sh: Status values do not equal"
	    exit 1
	fi
	#else fall through, exit 0

    #if the status was 139, print a message
    else
	echo "ps_validate.sh: original program exits with 139 status with no input."
	#fall through for now, exit 0
    fi
fi

echo "ps_validate.sh: All inputs validated"
exit 0
