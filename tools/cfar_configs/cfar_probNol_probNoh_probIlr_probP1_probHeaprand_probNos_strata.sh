#!/bin/bash 
# nol, noh not set here, passed only to gen_mvee_config.
$PEASOUP_HOME/tools/cfar.sh "$@" --backend strata --step non_overlapping_stack=on --step ilr=on --step p1transform=on ---step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" -config_name $(basename $0 .sh|sed "s/cfar_//") --diehard
