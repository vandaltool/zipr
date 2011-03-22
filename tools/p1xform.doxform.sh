#!/bin/sh
#
# $1 is a file containing list of functions to p1-xform
# $2 specifies directory containing all the assembly SPRI rules
#
# Output: 
#   Binary spri file specification for P1 xform 
#

echo "=========================================="
echo "Running p1xform.pbed.sh"
echo "=========================================="

FNS=$1             # file containing name of functions to evaluate
ASPRI_DIR=$2       # directory with assembly SPRI rules

NEW_ASPRI_FILE=$ASPRI_DIR/p1.final.aspri
touch $NEW_ASPRI_FILE

while read fn;
do
  cat $ASPRI_DIR/p1.$fn.aspri >> $NEW_ASPRI_FILE
done < $FNS

$STRATA_REWRITE/tools/spasm/spasm $NEW_ASPRI_FILE p1.final.bspri
