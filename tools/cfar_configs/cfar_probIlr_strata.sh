#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$1" "$2" "$3" --backend strata --step ilr=on --config_name $(basename $0 .sh|sed "s/cfar_//")
