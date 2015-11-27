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

	echo "Found strata backend."

#
# note that these are all hard-coded in the config file right now.
# plan:  mine these values out of ps_run.sh and replace in config file.
#
#"STRATA_LOG=detectors",
#"PEASOUP_SCHEDULE_PERTURB=0",
#"STRATA_WATCHDOG=0",
#"STRATA_NUM_HANDLE=0",
#"STRATA_DOUBLE_FREE=0",
#"STRATA_HEAPRAND=0",
#"STRATA_SHADOW_STACK=0",
#"STRATA_CONTROLLED_EXIT=0",
#"STRATA_DETECT_SERVERS=0",
#"STRATA_PC_CONFINE=0",
#"STRATA_PC_CONFINE_XOR=0",
#"STRATA_REKEY_AFTER=0",
#"STRATA_PC_CONFINE_XOR_KEY_LENGTH=1024",
#"STRATA_IS_SO=0",
#"STRATA_MAX_WARNINGS=500000"

	base_peasoup_dir=$(basename $newdir)

	cat $PEASOUP_HOME/tools/cfar_configs/strata_variant.json.template| sed "s|<<PS_DIR>>|$base_peasoup_dir|" > variant_config.json
	exit 0

else
	echo Unknown backend.
	exit 1
fi

