#!/bin/sh

#
# Assumption: we're in the top level directory created by the peasoup toolchain
#
# Replay integer transform using manual regression tests
#
# Warning: no sandboxing is performed, make sure regression tests are non-destructive
#

# Inputs
REGRESSION_TEST_SCRIPT=$1            # path of regression test script
STRATAFIED_BINARY=$2                 # stratafied subject program (a.stratafied)
BSPRI=$3                             # bspri file with integer instrumention (warnings)
CUMUL_DIAGNOSTICS=$4                 # path of file containing cumulated diagnostics

# Output
INTEGER_WARN_INSTRUCTIONS=$5         # output file with addresses of benign errors

TOP_LEVEL=`pwd`
REGRESSION_TEST_SCRIPT_TIMEOUT=600   # timeout value for regression tests (seconds)

echo "=========================================="
echo "Running intxform_replay.sh"
echo "      REGRESSION_TEST_SCRIPT: $REGRESSION_TEST_SCRIPT"
echo "           STRATAFIED_BINARY: $STRATAFIED_BINARY"
echo "                       BSPRI: $BSPRI"
echo "           CUMUL_DIAGNOSTICS: $CUMUL_DIAGNOSTICS"
echo "   INTEGER_WARN_INSTRUCTIONS: $INTEGER_WARN_INSTRUCTIONS (output file)"
echo "                         DIR: $TOP_LEVEL"
echo "=========================================="

#
# Algorithm:
# (1) run regression tests against integer transformed binary in diagnostics mode
# (2) extract address from diagnostics  
# (3) produce list of address where the instruction results in a benign false positive
#
# Warning: 
# We assume that the regression tests consists of trusted inputs
#

# (1) run regression tests against integer transformed binary in diagnostics mode
LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$TOP_LEVEL" STRATA_LOG=detectors STRATA_OUTPUT_FILE="$TOP_LEVEL/diagnostics.out" STRATA_IS_SO=0 STRATA_ANNOT_FILE="$TOP_LEVEL/a.ncexe.annot" STRATA_PC_CONFINE=1 STRATA_DETECTOR_POLICY="continue" STRATA_SPRI_FILE="$BSPRI" STRATA_NUM_HANDLE=1 STRATA_SIEVE=1 STRATA_RC=1 STRATA_PARTIAL_INLINING=0 STRATA_EXE_FILE="$TOP_LEVEL/a.stratafied" STRATA_DOUBLE_FREE=1 STRATA_MAX_WARNINGS=50000 timeout $REGRESSION_TEST_SCRIPT_TIMEOUT $REGRESSION_TEST_SCRIPT -i $STRATAFIED_BINARY $STRATAFIED_BINARY  

# Produce final output file containing addresses of detected benign false positive
# (2) extract address from diagnostics  
# (3) produce list of address where the instruction results in a benign false positive
touch $INTEGER_WARN_INSTRUCTIONS
cat $CUMUL_DIAGNOSTICS | grep -i diagnos | grep class | grep C1 | sed 's/.*diagnosis.*PC:\(.*\)/\1/' | sort | uniq | cut -d' ' -f1 >> $INTEGER_WARN_INSTRUCTIONS
