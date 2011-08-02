#!/bin/sh

#
# This is the main driver script for the P1 transform
# Run this script from top-level directory created by the peasoup script
#

CURRENT_DIR=`pwd`

P1_DIR=p1.xform

mkdir $P1_DIR

ASPRI_DIR=$P1_DIR/aspri
BSPRI_DIR=$P1_DIR/bspri

echo ""
echo "=========================================="
echo "p1xform.sh script started in $CURRENT_DIR"
echo "P1 transform directory: $P1_DIR"
echo "=========================================="

echo "==========================================================="
echo "Generate P1 transformation for each function in the program"
echo "==========================================================="
$PEASOUP_HOME/tools/p1xform.genspri.sh $P1_DIR a.ncexe a.ncexe.annot > $P1_DIR/genspri.out 2> $P1_DIR/genspri.err

#
# remove any candidate functions not covered
# this will go away once GrACE gives us the instruction coverage information
#
CONCOLIC=concolic.files_a.stratafied_0001

COVERAGE_FNS=$P1_DIR/p1.coverage
CANDIDATE_FNS=$P1_DIR/p1.candidates
FILTERED_OUT=$P1_DIR/p1.fn_coverage.filtered_out
KEEPS=$P1_DIR/p1.keep
FINAL_XFORM_FNS=$P1_DIR/p1.final
EXECUTED_ADDRESS_FILE=$CONCOLIC/executed_address_list.txt
LIBC_FILTER=$PEASOUP_HOME/tools/p1xform.filter.libc.txt

#
# Prune out functions that do not have sufficient coverage
# Any function whose coverage metric starts with 0.0, e.g. 0.09, 0.0123, is pruned out
# We effectively prune out any functions whose coverage is not at least 10% 
#
$SECURITY_TRANSFORMS_HOME/tools/cover/cover a.ncexe a.ncexe.annot $EXECUTED_ADDRESS_FILE $COVERAGE_FNS
grep -v "0\.0" $COVERAGE_FNS | cut -f1 -d" " > $CANDIDATE_FNS
grep  "0\.0" $COVERAGE_FNS | cut -f1 -d" " > $FILTERED_OUT
#rm tmp.$$

# Prune out libc functions
$PEASOUP_HOME/tools/p1xform.filter.sh $CANDIDATE_FNS $LIBC_FILTER > $KEEPS

echo "====================================================="
echo "Run BED"
echo "====================================================="
cd $CURRENT_DIR
$PEASOUP_HOME/tools/p1xform.pbed.sh $P1_DIR $KEEPS $CONCOLIC $BSPRI_DIR $FINAL_XFORM_FNS

echo "====================================================="
echo "Produce final transformed binary"
echo "====================================================="
cd $CURRENT_DIR
$PEASOUP_HOME/tools/p1xform.doxform.sh $FINAL_XFORM_FNS $P1_DIR $ASPRI_DIR

echo "====================================================="
echo "Validate final transformed binary"
echo "====================================================="
if [ -f $P1_DIR/p1.final.bspri ]; then
  $PEASOUP_HOME/tools/ps_validate.sh ./a.stratafied $P1_DIR/p1.final.bspri $CONCOLIC replay.baseline > ps_validate.out 2> ps_validate.err
  if [ $? -eq 0 ]; then
    echo "Successfully validated p1-transformed functions against inputs"
    echo "The following functions were transformed:"
    cat $P1_DIR/p1.final
  else
    echo "Did not successfully validate p1-transformed functions against inputs"
  fi
else
  echo "Unable to use p1 transform -- no rules produced"
fi

