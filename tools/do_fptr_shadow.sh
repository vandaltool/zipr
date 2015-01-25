#!/bin/bash -x
#
# do_fptr_shadow <cloneId>
#

# input
CLONE_ID=$1

#
# hack for now b/c STARS does not yet emit FPTR annotations
# copy from directory above
#
FPTR_ANNOT_FILE=a.ncexe.fptrannot
cp /tmp/$FPTR_ANNOT_FILE .

# weave-in calls to callback handlers for shadowing
$SECURITY_TRANSFORMS_HOME/tools/fptr_shadow/fptr_shadow64.exe $CLONE_ID
