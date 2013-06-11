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
ORIG_BINARY=$3                       # original binary (a.ncexe)
BSPRI=$4                             # bspri file with integer instrumention (warnings)
CUMUL_DIAGNOSTICS=$5                 # path of file containing cumulated diagnostics

# Output
INTEGER_WARN_INSTRUCTIONS=$6         # output file with addresses of benign errors

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

touch $CUMUL_DIAGNOSTICS
touch $INTEGER_WARN_INSTRUCTIONS

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
timeout $REGRESSION_TEST_SCRIPT_TIMEOUT $REGRESSION_TEST_SCRIPT -i $STRATAFIED_BINARY $ORIG_BINARY  

# Produce final output file containing addresses of detected benign false positive
# (2) extract address from diagnostics  
# (3) produce list of unique addresses where the instructions result in a benign false positive
cat $CUMUL_DIAGNOSTICS | grep -i diagnos | grep class | grep C1 | sed 's/.*diagnosis.*PC:\(.*\)/\1/' | cut -d' ' -f1 | sort | uniq >> $INTEGER_WARN_INSTRUCTIONS
