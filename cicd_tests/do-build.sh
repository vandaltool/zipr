#/bin/bash 

set -e
set -x

main()
{

	# gather info for debugging later, probably not necessary 
	pwd
	hostname
	whoami
	env|grep "^CICD"

	git submodule sync 
	git submodule update --init --recursive

	local orig_dir=$(pwd)

	# puts irdblibs_umbrella (and all submodules) in CICD_MODULE_WORK_DIR
	cicd_setup_module_dependency allnp/peasoup_umbrella.git irdblibs_umbrella


	# puts the version of irdb-libs to test in irdblibs_umbrella/irdb-libs
	cicd_put_module_in_tree irdblibs_umbrella/irdb-libs

	# Build/run $PSZ, test result
	cd $CICD_MODULE_WORK_DIR/irdblibs_umbrella
	source set_env_vars
	sudo ./get-peasoup-packages.sh all

	# remove pedi files so that rebuilding includes re-doing pedi setup.
	$PEDI_HOME/pedi -c -m manifest.txt || true # ignore errors in cleanup
	./build-all.sh
	dropdb $PGDATABASE 2>/dev/null || true ; ./postgres_setup.sh

	cd $orig_dir
}

main "$@"
