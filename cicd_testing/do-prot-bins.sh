#!/bin/bash

set -e 
set -x

pushd /tmp/peasoup_test
export IDAROOT=$CICD_MODULE_WORK_DIR/idapro82sp1py3
export IDASDK=$CICD_MODULE_WORK_DIR/idapro82_sdk
source set_env_vars
popd


install_deps()
{
	sudo apt-get install -y binutils-aarch64-linux-gnu gcc-aarch64-linux-gnu
}

main()
{
	install_deps
	export PS_OBJDUMP=$(which aarch64-linux-gnu-objdump)
	export PS_READELF=$(which aarch64-linux-gnu-readelf)
	export PS_GCC=$(which aarch64-linux-gnu-gcc)
	export PS_OBJCOPY=$(which aarch64-linux-gnu-objcopy)


	mkdir -p artifacts/protected_binaries
	for i in $(ls artifacts/test_binaries/); do
		$PSZ  artifacts/test_binaries/$i artifacts/protected_binaries/$(basename $i.$(hostname))
	done
}


main "$@"
