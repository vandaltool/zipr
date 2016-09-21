#!/bin/bash -x


echo \$USER is $USER
#Create Database User and Table
source set_env_vars
for i in $(seq 0 4)
do
	dropdb peasoup_${USER}_v$i
	createdb -O ${USER} peasoup_${USER}_v$i

	#Setup the Database to store PEASOUP info
	PGDATABASE=peasoup_${USER}_v$i $PEASOUP_HOME/tools/db/pdb_setup.sh
done
