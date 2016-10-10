#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step diehard=on --step p1transform=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step move_globals=on --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --step-option zipr:"--large_only:nog_on true" --step noh=on --step nol=on --step stack_stamp=on

