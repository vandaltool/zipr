#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_noc --structured_nos --step-option zipr:"--large_only:on true" --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --step nol=on --structured_p1_canaries  --step p1transform=on 
