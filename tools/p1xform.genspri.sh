#!/bin/sh

P1_DIR=$1   # directory where to stash away all things related to the P1 xform 
BINARY=$2   # path of subject binary program
ANNOT=$3    # path of annotations for the original binary

# produce list of candidate functions
# produce list of non-candidate functions
# produce bad asm SPRI rules for candidate functions
# produce good asm SPRI rules for candidate functions
#
# to do: split the above into distinct commands/options combinations

$SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform $BINARY $ANNOT 

ASPRI_DIR=$P1_DIR/aspri
BSPRI_DIR=$P1_DIR/bspri

mkdir $ASPRI_DIR 2>/dev/null
mkdir $BSPRI_DIR 2>/dev/null

echo ""
echo "==================================================="
echo "Generating initial spri files"
echo "   output aspri dir: $ASPRI_DIR"
echo "   output bspri dir: $BSPRI_DIR"
echo "==================================================="

# move files produced by the p1transform tool
mv *.aspri $ASPRI_DIR
mv p1.*candid* $P1_DIR

for i in `ls $ASPRI_DIR/*p1*.aspri`
do
  base=`basename $i .aspri`
  $SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $i $BSPRI_DIR/"$base".bspri  
done

echo "Done generating initial spri files"
echo "==================================================="
