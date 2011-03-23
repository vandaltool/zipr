#!/bin/sh

#
# Validate startup/shutdown sequence by comparing 2 programs
# and feeding them no inputs
#

ORIG_BINARY=$1           # original binary
STRATAFIED_BINARY=$2     # stratafied binary
BSPRI=$3                 # transformation specificiation SPRI file

echo "=========================================="
echo "Running ps_validate_ss.sh"
echo "           ORIG_BINARY: $ORIG_BINARY"
echo "     STRATAFIED_BINARY: $STRATAFIED_BINARY"
echo "                 BSPRI: $BSPRI"
echo "=========================================="

$ORIG_BINARY > a0.orig.out 2> a0.orig.err
ORIG_VAL=$?

STRATA_SPRI_FILE="$BSPRI" $STRATAFIED_BINARY > a0.strata.out 2> a0.strata.err
STRATA_VAL=$?

if [ "$ORIG_VAL" = "$STRATA_VAL" ]; then
  exit 0
else
  echo "ps_validate_ss.sh: BSPRI=$BSPRI: does not validate ($ORIG_VAL/$STRATA_VAL)" 
  exit 1
fi
