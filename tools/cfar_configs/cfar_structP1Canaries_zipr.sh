#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$1" "$2" "$3" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_p1_canaries  --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//")
