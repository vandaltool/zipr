#!/bin/bash 

SPASM_SEED=$$ $PEASOUP_HOME/tools/cfar.sh "$@" --step p1transform=on   --config_name $(basename $0 .sh|sed "s/cfar_//")
