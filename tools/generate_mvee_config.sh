#!/bin/bash

echo Generating stuffs.

variants=$1 
outfile=$2 
tempdir=$3
backend=$4

json=${outfile}.json

if [ "$backend" = 'zipr' ]; then
	cp $PEASOUP_HOME/tools/cfar_configs/zipr_all.json.template $json

	json_contents=$(<$json)

	for seq in $(seq 0 $(expr $variants - 1))
	do
		variant_config_contents=$(<$tempdir.v${seq}/variant_config.json)
		variant_name="variant_v${seq}"
		updated_config="${variant_config_contents//<<VARIANTNUM>>/$variant_name}"
		json_contents="${json_contents//<<VARIANT_CONFIG>>/$updated_config,<<VARIANT_CONFIG>>}"
		json_contents="${json_contents//<<VARIANT_LIST>>/\"$variant_name\",<<VARIANT_LIST>>}"

	done

	# remove variant_config marker.
	json_contents="${json_contents//,<<VARIANT_CONFIG>>/}"
	json_contents="${json_contents//,<<VARIANT_LIST>>/}"

	echo "$json_contents" > $json
	
elif [ "$backend" = 'strata' ]; then
	echo strata not done yet
	exit 1
else
	echo unknown backend: $backend
	exit 1
fi
