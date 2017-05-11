#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_p1_canaries  --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//")
