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
# --crash_dir=<fully_qualified_path_to_pov_dir>
# --pov_crash_summary=<fully_qualified_path_to_crash_summary>
# 

ORIG=$1
CGC_CSID=$2
CRASH_CSO_FILE=$3

shift 3

short_opts="p:c:d:"
long_opts="--long pov_dir: --long pov_crash_summary: --long crash_dir:"

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
		--crash_dir | d)
			CRASH_DIR=$2
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

if [ ! -f $POV_CRASH_SUMMARY ]; then
	touch $POV_CRASH_SUMMARY
fi


TMP_DIR=tmp.dir.pov.crash.$$
TMP_FILE=tmp.file.$$

if [ -z $POV_DIR ]; then
	mkdir $TMP_DIR
	POV_DIR=`pwd`/$TMP_DIR
fi

if [ -z $CRASH_DIR ]; then
	mkdir $TMP_DIR
	CRASH_DIR=`pwd`/$TMP_DIR
fi

if [ -z $POV_CRASH_SUMMARY ]; then
	touch $TMP_FILE
	POV_CRASH_SUMMARY=`pwd`/$TMP_FILE
fi

echo "POV_DIR: $POV_DIR"
echo "POV_CRASH_SUMMARY: $POV_CRASH_SUMMARY"
echo "CRASH_DIR: $CRASH_DIR"

echo "cmd: $SECURITY_TRANSFORMS_HOME/tools/cgc_protect/pov_to_cso.sh $ORIG $CGC_CSID $POV_DIR $CRASH_CSO_FILE $POV_CRASH_SUMMARY $CRASH_DIR"

$SECURITY_TRANSFORMS_HOME/tools/cgc_protect/pov_to_cso.sh $ORIG $CGC_CSID $POV_DIR $CRASH_CSO_FILE $POV_CRASH_SUMMARY $CRASH_DIR

rmdir $TMP_DIR &>/dev/null
rm $TMP_FILE &>/dev/null

exit 0

