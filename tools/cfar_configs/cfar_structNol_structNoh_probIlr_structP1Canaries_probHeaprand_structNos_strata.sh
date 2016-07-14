#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend strata --step ilr=on --structured_p1_canaries --structured_nos --step non_overlapping_stack=on --step p1transform=on ---step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" -config_name $(basename $0 .sh|sed "s/cfar_//") --diehard
