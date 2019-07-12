#!/bin/bash

set -e 
set -x

pushd /tmp/peasoup_test
source set_env_vars
popd


mkdir -p artifacts/protected_binaries

main()
{
	for i in $(ls artifacts/test_binaries/); do
		$PSZ  $i artifacts/protected_binaries/$(basename $i.$(hostname))
	done
}


main "$@"
