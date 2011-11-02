#!/bin/sh
#
# do_p1transform.sh <cloneId> 
#
# pre: we are in the top-level directory created by ps_analyze.sh
#

# input
CLONE_ID=$1

# configuration variables
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt   # libc and other system library functions
ANNOT_INFO=a.ncexe.infoannot                         # new annotation for integer checks

echo "INT: transforming binary: cloneid=$CLONE_ID annotationInfoFile=$ANNOT_INFO"

$SECURITY_TRANSFORMS_HOME/tools/transforms/integertransformdriver.exe $CLONE_ID $ANNOT_INFO $LIBC_FILTER
