#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$1" "$2" "$3" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_noc --step-option zipr:"--large_only:on true"
