#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --diehard --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step diehard=on
