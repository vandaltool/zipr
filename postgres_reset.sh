#!/bin/bash


main()
{
	dropdb peasoup_$USER || true
	createdb peasoup_$USER 

	#Setup the Database to store PEASOUP info
	source set_env_vars
	$PEASOUP_HOME/tools/db/pdb_setup.sh
}

main 
