#!/bin/bash

#Create Database User and Table
source set_env_vars
for i in $(seq 0 4)
do
	sudo su -c "createdb -O $USER peasoup_$USER_v$i" postgres

	#Setup the Database to store PEASOUP info
	PGDATABASE=peasoup_$USER_v$i $PEASOUP_HOME/tools/db/pdb_setup.sh

done
