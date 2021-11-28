#!/bin/bash -x


echo \$USER is $USER


reset_db()
{
	dropdb $PGDATABASE || true
	createdb -O ${USER} $PGDATABASE
	$PEASOUP_HOME/tools/db/pdb_setup.sh
}


# reset default db
reset_db

#Create Database User and Table
for i in $(seq 0 128)
do
	#Setup the Database to store PEASOUP info
	PGDATABASE=peasoup_${USER}_v$i reset_db
done
