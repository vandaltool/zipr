#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" \
	--backend zipr --structured_noc --step-option zipr:"--zipr:seed $$" --step-option zipr:"--large_only:on true"  \
	--step p1transform=on  --structured_p1_canaries  \
	--step stack_stamp=on --structured_stack_stamp \
	--step diehard=on  --structured_heap  \
	--config_name $(basename $0 .sh|sed "s/cfar_//")
