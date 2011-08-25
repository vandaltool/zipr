#!/bin/sh
#
# do_p1transform.sh <cloneId> 
#
# pre: we are in the top-level directory created by ps_analyze.sh
#

# input
CLONE_ID=$1

# configuration variables
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt

echo "INT: transforming binary: cloneid=$CLONE_ID"

$SECURITY_TRANSFORMS_HOME/tools/transforms/integerbugtransform.exe $CLONE_ID $LIBC_FILTER
