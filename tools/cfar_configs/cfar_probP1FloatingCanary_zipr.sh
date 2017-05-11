#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step p1transform=on --step-option p1transform:"--canaries=on --floating_canary" --step-option zipr:"--zipr:seed $$" --config_name $(basename $0 .sh|sed "s/cfar_//")
