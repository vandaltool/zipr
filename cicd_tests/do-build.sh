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

	local orig_dir=$(pwd)

	# puts peasoup_umbrella (and all submodules) in CICD_MODULE_WORK_DIR
	cicd_setup_module_dependency allnp/peasoup_umbrella.git zipr_umbrella


	# puts the version of zipr to test in peasoup_umbrella/zipr.
	cicd_put_module_in_tree zipr_umbrella/zipr

	# Build/run $PSZ, test result
	cd $CICD_MODULE_WORK_DIR/zipr_umbrella
	source set_env_vars
	sudo ./get-peasoup-packages.sh all
	./build-all.sh
	dropdb $PGDATABASE 2>/dev/null || true ; ./postgres_setup.sh

	cd $orig_dir
}

main "$@"
