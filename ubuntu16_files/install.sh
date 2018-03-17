#!/bin/bash 


main()
{
	# 1) force sudo password as necessary
	# 2) make sure package manager is ready for installing packages.
	sudo apt-get update

	local packages="
		realpath
		gcc
		nasm
		coreutils
		postgresql
		postgresql-client
		libpqxx-4.0
		python
		"

	for i in $packages
	do
		sudo  apt-get install -y $i 
	done 
}

main "$@"
