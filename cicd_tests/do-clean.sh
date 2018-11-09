#/bin/bash 

set -e
set -x

main()
{

	# gather info for debugging later, probably not necessary 
	pwd
	hostname
	whoami
	env|grep CICD


	if [[ $CICD_NIGHTLY == 1 ]] ; then
		rm -rf $CICD_MODULE_WORK_DIR/peasoup_umbrella
	fi

}

main "$@"
