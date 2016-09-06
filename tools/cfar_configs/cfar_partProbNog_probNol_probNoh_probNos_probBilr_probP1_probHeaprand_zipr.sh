#!/bin/bash 

file $2|grep "ELF 64-bit LSB  executable" > /dev/null
if [ $? = 0 ]; then
	$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step p1transform=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step move_globals=on --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --step-option zipr:"--large_only:nog_on true" --step diehard=on --step noh=on --step nol=on
else
	$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step p1transform=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step diehard=on --step noh=on --step nol=on
fi
