#!/bin/sh
#
# $1 is a file containing list of functions to p1-xform
# $2 specifies directory containing all the assembly SPRI rules
#
# Output: 
#   Binary spri file specification for P1 xform 
#

FNS=$1             # file containing name of functions to evaluate
P1_DIR=$2       # directory with assembly SPRI rules
ASPRI_DIR=$3       # directory with assembly SPRI rules
FINAL_BSPRI_FILE=$P1_DIR/p1.final.bspri

echo "=========================================="
echo "Running p1xform.doxform.sh"
echo "            FNS: $FNS"
echo "         P1_DIR: $P1_DIR"
echo "      ASPRI_DIR: $ASPRI_DIR"
echo "------------------------------------------"
echo "    Output File: $FINAL_BSPRI_FILE"
echo "=========================================="

NEW_ASPRI_FILE=$ASPRI_DIR/p1.final.aspri
touch $NEW_ASPRI_FILE

while read fn;
do
  cat $ASPRI_DIR/p1.$fn.aspri >> $NEW_ASPRI_FILE
done < $FNS

echo "p1xform.doxform.sh: issuing cmd: $STRATA_REWRITE/tools/spasm/spasm $NEW_ASPRI_FILE p1.final.bspri"
$STRATA_REWRITE/tools/spasm/spasm $NEW_ASPRI_FILE $FINAL_BSPRI_FILE

if [ -z $FINAL_BSPRI_FILE ]; then
  echo "p1xform.doxform.sh: Warning: no transforms specified for P1 algorithm"
  rm $FINAL_BSPRI_FILE
fi
