#!/bin/sh
#
# Generate file containing list of good functions
#
# $1 is a file containing list of functions to test
# $2 specifies directory with the inputs found by GrACE
# $3 specifies directory containing all the assembly SPRI rules
# $4 specifies directory containing all the binary SPRI rules
#
# Output: 
#   File with list of good functions
#


P1DIR=$1        # top-level P1 xform directory 
FNS=$2          # file containing name of functions to evaluate
INPUT_DIR=$3    # directory with inputs
BSPRI_DIR=$4    # directory with binary SPRI rules
P1_GOOD_FILE=$5 # final output file containing list of transformed functions

touch $P1_GOOD_FILE

echo "=========================================="
echo "Running p1xform.pbed.sh"
echo "P1DIR=$1"
echo "FNS=$2" 
echo "INPUT_DIR=$3"
echo "BSPRI=$4"
echo "------------------------------------------"
echo "Output File: $P1_GOOD_FILE"
echo "=========================================="

while read fn;
do
  echo "Checking for divergence on function $fn"

  BSPRI_GOOD="$BSPRI_DIR/p1.$fn.bspri"

  $PEASOUP_HOME/tools/ps_validate_ss.sh ./a.ncexe ./a.stratafied $BSPRI_GOOD 
  if [ ! $? -eq 0 ]; then
    continue
  fi

  $PEASOUP_HOME/tools/ps_validate.sh ./a.stratafied $BSPRI_GOOD $INPUT_DIR replay.baseline
  if [ $? -eq 0 ]; then
    echo $fn >> $P1_GOOD_FILE
  fi

done < $FNS

