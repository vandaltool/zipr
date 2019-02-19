#!/bin/bash -x
source $(dirname $0)/../ps_wrapper.source $0


if [ ! -z $NO_FLOAT ];
then
	$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_p1_canaries --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//")
else
	$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --step-option zipr:"--zipr:seed $$" --structured_p1_canaries --step p1transform=on --step-option p1transform:"--floating_canary" --config_name $(basename $0 .sh|sed "s/cfar_//")
fi
