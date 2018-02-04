#!/bin/bash 

source $(dirname $0)/../ps_wrapper.source $0

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --config_name $(basename $0 .sh|sed "s/cfar_//") --step move_globals=on --step xor_globals=on --step-option move_globals:"--aggressive" --step-option move_globals:-d --step-option move_globals:.interp --step-option zipr:"--large_only:nog_on true"

