#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --diehard --step p1transform=on --step non_overlapping_stack=on --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --config_name $(basename $0 .sh|sed "s/cfar_//")
