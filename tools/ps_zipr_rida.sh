#!/bin/bash 

source $(dirname $0)/ps_wrapper.source $0
$PEASOUP_HOME/tools/ps_analyze.sh "$@" --backend zipr -s meds_static=off -s rida=on
