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
#
# --pov_dir=<fully_qualified_path_to_pov_dir>
# --pov_crash_summary=<fully_qualified_path_to_crash_summary>
# 

ORIG=$1
CGC_CSID=$2
CRASH_CSO_FILE=$3

shift 3

short_opts="p:c:"
long_opts="--long pov_dir: --long pov_crash_summary:"

TEMP=`getopt -o $short_opts $long_opts -n 'do_protect_pov.sh' -- "$@"`
if [ ! $? -eq 0 ]; then
    echo "Error parsing options for do_protect_pov.sh: $ORIG $CGC_CSID $CRASH_CSO_FILE $@"
    exit 1
fi

eval set -- "$TEMP"

while true ; do
	case "$1" in
		--pov_dir | p)
			POV_DIR=$2
			shift 2
		;;
		--pov_crash_summary | c)
			POV_CRASH_SUMMARY=$2
			shift 2
		;;
		--) 	shift 
			break 
		;;
	esac
done

echo "POV_DIR: $POV_DIR"
echo "POV_CRASH_SUMMARY: $POV_CRASH_SUMMARY"

if [ -z $POV_DIR ]; then
	echo "ERROR: No POV directory was specified"
	return 1
fi

if [ ! -f $POV_CRASH_SUMMARY ]; then
	touch $POV_CRASH_SUMMARY
fi

echo "cmd: $SECURITY_TRANSFORMS_HOME/tools/cgc_protect/pov_to_cso.sh $ORIG $CGC_CSID $POV_DIR $CRASH_CSO_FILE $POV_CRASH_SUMMARY"

$SECURITY_TRANSFORMS_HOME/tools/cgc_protect/pov_to_cso.sh $ORIG $CGC_CSID $POV_DIR $CRASH_CSO_FILE $POV_CRASH_SUMMARY

exit 0

