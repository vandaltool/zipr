#!/bin/bash 

function main()
{
	set -e 
	# build software
	git submodule sync --recursive
	git submodule update --recursive --init

	scons -j3

	# force reinstall tools so we are always up-to-date
	yes | sudo bash -c "$(curl -fsSL allzp.zephyr-software.io/turbo/cli-install.sh)"

	# better done with boost add -q -i 
	turbo-cli boost add rida_exe || true
	local bid=$(turbo-cli boost list|grep rida_exe|cut -d"	" -f1)

	# add seeds, ignore errors if they already exist.
	turbo-cli seed add $bid cicd_testing/rida-seed.yaml || true
	turbo-cli seed add $bid cicd_testing/rida-seed2.yaml || true
	turbo-cli seed add $bid cicd_testing/rida-seed3.yaml || true
	turbo-cli seed add $bid cicd_testing/rida-seed4.yaml || true

	local vid=$(turbo-cli version add -q $bid irdb-libs/plugins_install/rida.exe)
	turbo-cli fuzz --fuzz-config cicd_testing/afl.yaml --app-config cicd_testing/rida-config.yaml --ver-id $vid

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
