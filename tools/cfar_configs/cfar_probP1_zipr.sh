#!/bin/bash 

$PEASOUP_HOME/tools/cfar.sh "$1" "$2" "$3" --backend zipr --step p1transform=on --step-option zipr:"--zipr:seed $$"
