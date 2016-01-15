#!/bin/bash

echo Generating stuffs.

variants="$1"
outfile="$2"
tempdir="$3"
backend="$4"
config="$5"
use_diehard="$6"

json=${outfile}.${config}.json

if [ "$backend" = 'zipr' ]; then
	cp $PEASOUP_HOME/tools/cfar_configs/zipr_all.json.template $json
elif [ "$backend" = 'strata' ]; then
	cp $PEASOUP_HOME/tools/cfar_configs/strata_all.json.template $json
else
	echo unknown backend: $backend
	exit 1
fi

json_contents=$(<$json)

for seq in $(seq 0 $(expr $variants - 1))
do
#-	variant_config_contents=$(<$tempdir.v${seq}/variant_config.json)
#-	variant_name="variant_v${seq}"
#+	variant_config_contents=$(<$tempdir/v${seq}/peasoup_executable_dir/variant_config.json)
#+	variant_name="$config_${seq}"

	variant_config_contents=$(<$tempdir.v${seq}/variant_config.json)
	variant_name="variant_v${seq}"
	updated_config="${variant_config_contents//<<VARIANTNUM>>/$variant_name}"
	json_contents="${json_contents//<<VARIANT_CONFIG>>/$updated_config,<<VARIANT_CONFIG>>}"
	json_contents="${json_contents//<<VARIANT_LIST>>/\"$variant_name\",<<VARIANT_LIST>>}"

done

if [ $use_diehard ]; then
	json_contents="${json_contents//<<ENV>>/\"LD_PRELOAD=\/variant_specific\/libheaprand.so\",<<ENV>>}"
fi

# remove variant_config marker.
json_contents="${json_contents//,<<VARIANT_CONFIG>>/}"
json_contents="${json_contents//,<<VARIANT_LIST>>/}"
json_contents="${json_contents//,<<ENV>>/}"
json_contents="${json_contents//<<ENV>>/}"

echo "$json_contents" > $json

