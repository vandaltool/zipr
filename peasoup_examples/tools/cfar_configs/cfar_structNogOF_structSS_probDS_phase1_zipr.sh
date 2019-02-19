#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step diehard=on --step-option zipr:"--zipr:seed $$" --structured_heap --structured_noc --structured_nos --step-option zipr:"--large_only:on true" --structured_p1_canaries  --step p1transform=on --step duck_season=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step move_globals=on --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --structured_nog --step-option zipr:"--large_only:nog_on true --large_only:overflow_protection true" --step noh=on --step nol=on --step stack_stamp=on --structured_stack_stamp --step-option move_globals:-d --step-option move_globals:.interp --step-option move_globals:--aggressive

