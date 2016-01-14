#!/bin/bash -x

#Create Database User and Table
source set_env_vars

other=$1



echo other is $other

sudo su -c "dropdb peasoup_${other}" postgres
sudo su -c "createdb -O ${other} peasoup_${other}" postgres
PGDATABASE=peasoup_${other} $PEASOUP_HOME/tools/db/pdb_setup.sh

for i in $(seq 0 4)
do
	sudo su -c "dropdb peasoup_${other}_v$i" postgres
	sudo su -c "createdb -O ${other} peasoup_${other}_v$i" postgres

	#Setup the Database to store PEASOUP info
	PGDATABASE=peasoup_${other}_v$i $PEASOUP_HOME/tools/db/pdb_setup.sh

done
