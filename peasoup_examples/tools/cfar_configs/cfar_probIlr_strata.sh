#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


$PEASOUP_HOME/tools/cfar.sh "$@" --backend strata --step ilr=on --config_name $(basename $0 .sh|sed "s/cfar_//")