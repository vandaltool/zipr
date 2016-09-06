#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --config_name $(basename $0 .sh|sed "s/cfar_//") --step libtwitcher=on 
