#!/bin/bash 

function main()
{


	g++ test.cpp -I ../include -fmax-errors=2 -L../../lib -lMEDSannotation -g -o test.exe

	# force reinstall tools so we are always up-to-date
	yes | sudo bash -c "$(curl -fsSL allzp.zephyr-software.io/turbo/cli-install.sh)"

	# better done with boost add -q -i 
	turbo-cli boost add map_so || true
	local bid=$(turbo-cli boost list|grep map_so|cut -d"	" -f1)

	# add seeds, ignore errors if they already exist.
	turbo-cli seed add $bid ./map-seed1.yaml || true
	turbo-cli seed add $bid ./map-seed2.yaml || true
	turbo-cli seed add $bid ./map-seed3.yaml || true

	local vid=$(turbo-cli version add -q $bid ../../lib/libMEDSannotation.so)
	turbo-cli fuzz --fuzz-config ./afl.yaml --app-config ./map-config.yaml --ver-id $vid

	local report="$(turbo-cli log get report $vid)"

	echo "The report is: "
	echo "$report"

	local declare crash_count=$(echo "$report"|shyaml get-value failing-input-count)

	if [[ $crash_count == 0 ]]; then
		echo "No crashes found"
		exit 0
	else
		echo "$crash_count count crashes found!"
		exit 1
	fi

}

main "$@"
