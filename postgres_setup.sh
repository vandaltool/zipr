#!/bin/bash


main()
{
	local randpass=$(date +%s | sha256sum | base64 | head -c 32)

	#Add PostGres password file for modifying the database
	newlines=$(printf ":5432::$USER:$randpass\nlocalhost:5432:*:$USER:$randpass\n127.0.0.1:5432:*:$USER:$randpass\n" )
	echo -e "$newlines$(cat $HOME/.pgpass >/dev/null)" > $HOME/.pgpass
	chmod og-rw $HOME/.pgpass

	#Create Database User and Table
	echo "CREATE ROLE $USER WITH CREATEDB LOGIN NOSUPERUSER NOCREATEROLE PASSWORD '$randpass'" | sudo -u postgres psql
	echo "ALTER ROLE $USER WITH PASSWORD '$randpass'" | sudo -u postgres psql
	dropdb peasoup_$USER
	createdb peasoup_$USER

	#Setup the Database to store PEASOUP info
	source set_env_vars
	$PEASOUP_HOME/tools/db/pdb_setup.sh
}

main 
