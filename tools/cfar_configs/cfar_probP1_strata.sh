#!/bin/bash 
source $(dirname $0)/../ps_wrapper.source $0


SPASM_SEED=$$ $PEASOUP_HOME/tools/cfar.sh "$@" --step p1transform=on   --config_name $(basename $0 .sh|sed "s/cfar_//")
