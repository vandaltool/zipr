#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --diehard --step p1transform=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step move_globals=on --step-option zipr:"--large_only:nog_on true"
