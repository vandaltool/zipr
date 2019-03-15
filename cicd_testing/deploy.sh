#/bin/bash

export PS_PATH=git.zephyr-software.com:4567/opensrc/irdb-sdk/
export PS_TAG=zipr-bin:latest
export DOCKER_PS=${PS_PATH}${PS_TAG}


do_docker_clean()
{
	if [[ $CICD_WEEKLY == 1 ]]; then
		docker system prune -a -f
	fi
}


do_login()
{

	# login to gitlab's docker registry as gitlab-user
	docker login $PS_PATH -u gitlab-runner -p 84MyuSuDo4kQat4GZ_Zs  2> /dev/null
}

do_build_image()
{

	#
	# Re-install peasoup without ida.
	#
	cd $PEASOUP_HOME
	rm -rf installed || true
	$PEDI_HOME/pedi --setup -m manifest.txt -l ps -l zipr -l stars -i $PS_INSTALL
	$PEDI_HOME/pedi -m manifest.txt
	rm manifest.txt.config

	# build the docker image
	cd $PEASOUP_HOME/cicd_testing/docker-zipr-bin
	mv ../../installed .

	# if we fail here, continue on so we put "install" back in the right place.
	# the test should stop this
	docker build -t $DOCKER_PS . || true	
	mv installed ../..
}


do_push()
{
	if [[ $CICD_WEEKLY == 1 ]]; then
		docker push ${DOCKER_PS}
	fi
}

do_logout()
{
	docker logout $PS_PATH
}

main()
{
	if [[ -z $PEASOUP_HOME ]]; then
		cd /tmp/peasoup_test
		source set_env_vars
	fi

	set -e 
	do_docker_clean
	do_login
	do_build_image
	do_push
	do_logout
}

main "$@"
