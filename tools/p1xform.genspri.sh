#!/bin/sh

P1_DIR=$1   # directory where to stash away all things related to the P1 xform 
BINARY=$2   # full path of subject binary program
ANNOT=$3    # full path of annotations for the original binary

# produce list of candidate functions
# produce list of non-candidate functions
# produce bad asm SPRI rules for candidate functions
# produce good asm SPRI rules for candidate functions
#
# to do: split the above into distinct commands/options combinations

cd $P1_DIR

$STRATA_REWRITE/tools/transforms/p1transform $BINARY $ANNOT

ASPRI_DIR=$P1_DIR/aspri
BSPRI_DIR=$P1_DIR/bspri

mkdir $ASPRI_DIR
mkdir $BSPRI_DIR

echo ""
echo "==================================================="
echo "Generating initial spri files"
echo "   output aspri dir: $P1_DIR/$ASPRI"
echo "   output bspri dir: $P1_DIR/$BSPRI"
echo "==================================================="

mv *.aspri $ASPRI_DIR

for i in `ls $ASPRI_DIR/*p1*.aspri`
do
  base=`basename $i .aspri`
  $STRATA_REWRITE/tools/spasm/spasm $i $BSPRI_DIR/"$base".bspri  
done

echo "Done generating initial spri files"
echo "==================================================="
