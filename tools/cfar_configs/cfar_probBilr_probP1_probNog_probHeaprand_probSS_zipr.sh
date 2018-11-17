#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr -c diehard=on -c p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//") -c stack_stamp=on -c move_globals=on --step-option zipr:"--large_only:nog_on true" --step-option move_globals:-d --step-option move_globals:.interp --step-option move_globals:--aggressive

