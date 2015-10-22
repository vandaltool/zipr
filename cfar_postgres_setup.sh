#!/bin/bash -x


echo \$USER is $USER
#Create Database User and Table
source set_env_vars
for i in $(seq 0 4)
do
	sudo su -c "dropdb peasoup_${USER}_v$i" postgres
	sudo su -c "createdb -O ${USER} peasoup_${USER}_v$i" postgres

	#Setup the Database to store PEASOUP info
	PGDATABASE=peasoup_${USER}_v$i $PEASOUP_HOME/tools/db/pdb_setup.sh

done
