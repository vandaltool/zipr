#!/bin/sh

#
# This is the main driver script for the P1 transform
# Run this script from top-level directory created by the peasoup script
#

CURRENT_DIR=`pwd`

pidp=$1
fname=$2

P1_DIR=p1.xform/$fname

mkdir -p $P1_DIR

#generate the bspri code
$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $P1_DIR/a.irdb.aspri $P1_DIR/a.irdb.bspri $P1_DIR/stratafier.o.exe > $P1_DIR/spasm.out 2>&1

cat $P1_DIR/spasm.out

#
# remove any candidate functions not covered
# this will go away once GrACE gives us the instruction coverage information
#
CONCOLIC=concolic.files_a.stratafied_0001

echo "====================================================="
echo "Validate final transformed binary"
echo "====================================================="
if [ -f $P1_DIR/a.irdb.bspri ]; then
  $PEASOUP_HOME/tools/ps_validate.sh ./a.stratafied $P1_DIR/a.irdb.bspri $CONCOLIC > ps_validate.out 2> ps_validate.err
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

