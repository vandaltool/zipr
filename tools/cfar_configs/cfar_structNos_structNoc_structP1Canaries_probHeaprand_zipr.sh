#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --diehard --step-option zipr:"--zipr:seed $$" --structured_noc --structured_nos --step-option zipr:"--large_only:on true" --structured_p1_canaries  --step p1transform=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//")
