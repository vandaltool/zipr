#!/bin/bash -x

backend="zipr"

# check if strata was on from it's log.
if [  -f logs/stratafy_with_pc_confine.log ]; then
	backend="strata"
fi

if [ $backend = "zipr" ]; then
	echo Found zipr backend.
	exe=$(basename $stratafied_exe)
	cat $PEASOUP_HOME/tools/cfar_configs/zipr_variant.json.template| sed "s/<<EXE_NAME>>/$exe/" > variant_config.json


elif [ $backend = "strata" ]; then
	echo Found strata backend.
	exit 1

else
	echo Unknown backend.
	exit 1
fi

