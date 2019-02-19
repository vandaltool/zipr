#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr \
	-c p1transform=on \
	-c stack_stamp=on \
	-c move_globals=on \
	-c diehard=on \
	--structured_heap --structured_noc --structured_nog --structured_stack_stamp --structured_p1_canaries  \
	--step-option zipr:"--zipr:seed $$" --step-option zipr:"--large_only:on true" \
	--step-option zipr:"--large_only:nog_on true" \
	--step-option move_globals:-d --step-option move_globals:.interp --step-option move_globals:--aggressive \
	--config_name $(basename $0 .sh|sed "s/cfar_//") 
