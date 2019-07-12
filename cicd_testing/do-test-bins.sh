#!/bin/bash

set -e 
set -x

main()
{
	for i in $(ls artifacts/protected_binaries/*$(uname -m)*); do
		./artifacts/protected_binaries/$i --help
	done
}


main "$@"
