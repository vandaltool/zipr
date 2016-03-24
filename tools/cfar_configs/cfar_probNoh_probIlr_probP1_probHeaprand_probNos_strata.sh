#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend strata --step non_overlapping_stack=on --step ilr=on --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//") --diehard
