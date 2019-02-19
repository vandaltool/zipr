#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step diehard=on --step p1transform=on --step non_overlapping_stack=on --config_name $(basename $0 .sh|sed "s/cfar_//") --step noh=on