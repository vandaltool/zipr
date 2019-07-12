#!/bin/bash

set -e 
set -x

main()
{
	for i in $(ls artifacts/protected_binaries/); do
		$i --help
	done
}


main "$@"
