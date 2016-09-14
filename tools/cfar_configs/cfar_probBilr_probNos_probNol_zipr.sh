#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step p1transform=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --step nol=on
