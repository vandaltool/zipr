#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr -c diehard=on --step-option zipr:"--zipr:seed $$" --structured_heap --structured_noc --step-option zipr:"--large_only:on true" --structured_p1_canaries  -c p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//") -c stack_stamp=on --structured_stack_stamp -c move_globals=on --structured_nog --step-option zipr:"--large_only:nog_on true" --step-option move_globals:-d --step-option move_globals:.interp --step-option move_globals:--aggressive
