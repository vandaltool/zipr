#!/bin/bash

set -e 
set -x

mkdir -p artifacts/protected_binaries

main()
{
	for i in $(ls artifacts/test_binaries/); do
		$PSZ  $i artifacts/protected_binaries/$(basename $i.$(hostname))
	done
}


main "$@"
