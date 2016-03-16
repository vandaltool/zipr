#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend strata --step ilr=on --structured_p1_canaries --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//") --diehard
