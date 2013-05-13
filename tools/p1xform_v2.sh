#!/bin/sh

#
# This is the main driver script for the P1 transform
# Run this script from top-level directory created by the peasoup script
#

TOP_LEVEL=`pwd`

variantid=$1
aspri=$2
bspri=$3

P1_DIR=p1.xform/$fname

#generate the bspri code
$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $aspri $bspri $TOP_LEVEL/stratafier.o.exe $TOP_LEVEL/libstrata.so.symbols

if [ $? -ne 0 ]; then
	echo "Spasm failure in performing validation"
	exit 3
fi

$PEASOUP_HOME/tools/fast_spri.sh $bspri $TOP_LEVEL/a.irdb.fbspri

#
# remove any candidate functions not covered
# this will go away once GrACE gives us the instruction coverage information
#
CONCOLIC=$TOP_LEVEL/concolic.files_a.stratafied_0001

echo "====================================================="
echo "P1: Validating transformed binary..."
echo "====================================================="
if [ -f $bspri ]; then
  $PEASOUP_HOME/tools/ps_validate.sh ${TOP_LEVEL}/a.stratafied $TOP_LEVEL/a.irdb.fbspri $CONCOLIC > ps_validate.out 2> ps_validate.err
  if [ $? -eq 0 ]; then
      echo "Successfully validated p1-transformed functions against inputs"
      exit 0;
  else
      echo "Did not successfully validate p1-transformed functions against inputs"
      exit 1;
  fi
else
    echo "Unable to use p1 transform -- no rules produced"
    exit 2;
fi

