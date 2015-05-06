#!/bin/bash

#
# do_protect_pov.sh <orig> <csid> <output_cso_file> [ ... step_options ]
#
# Produces output warning file (CSO Format), e.g.:
# YAN01_00005,0x80485b0,,Tainted Dereference
# YAN01_00005,0x8040000,,Tainted Dereference
#
# This file can then be used by the sandboxing step

#
# pre: we're in the peasoup executable subdirectory
# in the step options:
# --pov_dir=<fully_qualified_path_to_pov_dir>
# 

ORIG=$1
CGC_CSID=$2
CRASH_CSO_FILE=$3
POV_DIR=$4

$SECURITY_TRANSFORMS_HOME/tools/cgc_protect/pov_to_cso.sh $ORIG $CGC_CSID $POV_DIR $CRASH_CSO_FILE

exit 0

