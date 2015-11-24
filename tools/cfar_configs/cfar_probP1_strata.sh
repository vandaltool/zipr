#!/bin/bash 

SPASM_SEED=$$ $PEASOUP_HOME/tools/cfar.sh "$1" "$2" "$3" --step p1transform=on   --config_name $(basename $0 .sh|sed "s/cfar_//")
