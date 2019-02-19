#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


#
# The calling script is responsible for setting up the options for duck_season
#    --json 
#    --imagename
#    [ --xor ]
#

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step diehard=on --step p1transform=on --step duck_season=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step initialize_stack=on --step-option initialize_stack:"--initvalue $$" --step move_globals=on --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --step-option zipr:"--large_only:nog_on true" --step noh=on --step nol=on --step stack_stamp=on  --step-option move_globals:-d --step-option move_globals:.interp --step-option move_globals:--aggressive --step-option duck_season:--xor 
