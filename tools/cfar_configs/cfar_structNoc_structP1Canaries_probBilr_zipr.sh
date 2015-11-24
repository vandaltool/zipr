#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$1" "$2" "$3" --backend zipr --structured_noc --step-option zipr:"--large_only:on true" --structured_p1_canaries  --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//")


# --step-option zipr:"--zipr:seed $$" 
