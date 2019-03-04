#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step diehard=on --step non_overlapping_stack=on --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --config_name $(basename $0 .sh|sed "s/cfar_//") --step noh=on --step nol=on
