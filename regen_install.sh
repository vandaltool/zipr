#!/bin/bash 


main()
{
	local keys="$@"

	local key_args=""
	for i in $keys
	do
		key_args="$key_args -l $i"
	done

	$PEDI_HOME/pedi --clean -m manifest.txt 
	$PEDI_HOME/pedi --setup -m manifest.txt $key_args -i $PS_INSTALL
	$PEDI_HOME/pedi -m manifest.txt 

}

main "$@"
