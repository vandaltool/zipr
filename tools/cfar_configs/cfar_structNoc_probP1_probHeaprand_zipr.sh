#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_noc --step-option zipr:"--large_only:on true" --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step diehard=on
