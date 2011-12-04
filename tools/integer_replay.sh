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
INPUT_DIR=$2                # directory containing inputs (.../concolic.files_a.stratafied_0001)
BSPRI=$3
INTEGER_WARN_INSTRUCTIONS=$4

BASELINE_OUTPUT_DIR=$INPUT_DIR/sandboxed-files

REPLAYER_TIMEOUT=120        # timeout value for when replaying input -- for now 120 seconds per input

ORIG_PROG=a.ncexe
baseline_cnt=0
TOP_LEVEL=`pwd`


REPLAY_DIR=replay_integer
rm -fr $REPLAY_DIR 2>/dev/null
mkdir $REPLAY_DIR 2>/dev/null

echo "=========================================="
echo "Running integer_replay.sh"
echo "                              DIR: $TOP_LEVEL"
echo "                STRATAFIED_BINARY: $STRATAFIED_BINARY"
echo "                            BSPRI: $BSPRI"
echo "         NTEGER_WARN_INSTRUCTIONS: $INTEGER_WARN_INSTRUCTIONS"
echo "             INPUT_DIR: $INPUT_DIR"
echo "   BASELINE_OUTPUT_DIR: $BASELINE_OUTPUT_DIR"
echo "=========================================="

#
# Do the NULL case
# Make sure we handle the case where program expects input off stdin
#
TMP=tmp.int.$$
rm -f $TMP
touch $TMP
STRATA_DETECTOR_POLICY="continue" STRATA_SPRI_FILE=$BSPRI STRATA_LOG=detectors STRATA_OUTPUT_FILE=integer.diagnostics timeout 60 $STRATAFIED_BINARY < $TMP

echo "Here's the diagnostics from running the NULL input case"
touch $INTEGER_WARN_INSTRUCTIONS
cat integer.diagnostics | sed 's/.*diagnosis.*PC:\(.*\)/\1/' | cut -d' ' -f1 >> $INTEGER_WARN_INSTRUCTIONS

rm $TMP

#
# Looks like the replayer is not compatible with strata + bspri
#

exit 0

#===================================
# FUNCTIONALITY BELOW DISABLED
#===================================

#
# name of files describing inputs is of the form: input_0001.json, input_0002.json, ...
#

for i in `ls $INPUT_DIR/input*.json`
do
 #Ben Modification: run replayer from top level to keep the sandbox layout the same as produced by the concolic test engine. This makes output comparison easier
  echo "Testing input spec: $i"
  input=`basename $i .json`
  input_number=`echo $input | sed "s/input_//"`
  abridged_number=`echo $input_number | sed 's/0*\(.*\)/\1/'`

  #at this point we know we have baseline data to compare against
  #we assume that if exit status was produced, baseline information is available
  baseline_cnt=`expr $baseline_cnt + 1`

  mkdir $REPLAY_DIR/$input_number 2>/dev/null
# make sure the output files exist
  touch $REPLAY_DIR/$input_number/stdout.$input
  touch $REPLAY_DIR/$input_number/stderr.$input

  #cleanup any previous runs
  rm -rf grace_replay
  rm -f stdout.* stderr.* exit_status

  echo "STRATA_DETECTOR_POLICY="continue" STRATA_LOG="detectors" STRATA_OUTPUT_FILE="integer.diagnostics.$input_number" STRATA_SPRI_FILE="$BSPRI" timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt $STRATAFIED_BINARY $i"
  STRATA_DETECTOR_POLICY="continue" STRATA_LOG="detectors" STRATA_OUTPUT_FILE="integer.diagnostics.$input_number" STRATA_SPRI_FILE="$BSPRI" timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt $STRATAFIED_BINARY $i 

  mv stderr.$input $REPLAY_DIR/$input_number/stderr.$input
  mv stdout.$input $REPLAY_DIR/$input_number/stdout.$input
  mv exit_status $REPLAY_DIR/$input_number/exit_status
  mv integer.diagnostics.$input_number $REPLAY_DIR/$input_number/

done
