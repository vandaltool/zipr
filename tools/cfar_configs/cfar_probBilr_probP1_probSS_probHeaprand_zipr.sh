#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr  \
	-c p1transform=on  \
	-c stack_stamp=on  \
	-c diehard=on  \
	--step-option zipr:"--large_only:nog_on true"  \
	--config_name $(basename $0 .sh|sed "s/cfar_//") 

