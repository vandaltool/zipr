#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$1" "$2" "$3" --backend zipr --step p1transform=on --step-option zipr:"--zipr:seed $$" --config_name $(basename $0 .sh|sed "s/cfar_//")
