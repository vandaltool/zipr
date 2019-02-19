#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step p1transform=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --step nol=on --step noh=on
