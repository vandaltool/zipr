#!/bin/bash 

if [ ! -z $NO_FLOAT ];
then
	$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_noc --structured_nos --step-option zipr:"--large_only:on true" --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --step nol=on --structured_p1_canaries  --step p1transform=on  --step noh=on
else
	$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_noc --structured_nos --step-option zipr:"--large_only:on true" --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step set_interpreter=on --step-option set_interpreter:"--interp /target_apps/ld-nol.so" --step nol=on --structured_p1_canaries --step p1transform=on  --step noh=on --step-option p1transform:"--floating_canary"
fi
