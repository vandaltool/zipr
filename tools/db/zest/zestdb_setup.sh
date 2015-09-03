#!/bin/bash
#
# Copyright (c) 2015 - Zephyr Software LLC
#
# This file may be used and modified for non-commercial purposes as long as
# all copyright, permission, and nonwarranty notices are preserved.
# Redistribution is prohibited without prior written consent from Zephyr
# Software.
#
# Please contact the authors for restrictions applying to commercial use.
#
# THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# Author: Zephyr Software
# e-mail: jwd@zephyr-software.com
# URL   : http://www.zephyr-software.com/
#

source $PEASOUP_HOME/tools/db/zest/setup_zestdb_env

PASSWD=q3z38zyt

function err_out {
	echo "zestdb_setup.sh: unbound environment variable: $1"
	exit 1
}

if [ -z $Z_PGUSER ]; then
	err_out Z_PGUSER
fi

if [ -z $Z_PGHOST ]; then
	err_out Z_PGHOST
fi

if [ -z $Z_PGPORT ]; then
	err_out Z_PGPORT
fi

if [ -z $Z_PGDATABASE ]; then
	err_out Z_PGDATABASE
fi

printf ":$Z_PGPORT::$Z_PGUSER:$PASSWD\nlocalhost:$Z_PGPORT:*:$Z_PGUSER:$PASSWD\n127.0.0.1:$Z_PGPORT:*:$Z_PGUSER:$PASSWD" >> $HOME/.pgpass
chmod og-rw $HOME/.pgpass

#Allow remote access to PostGres
sudo su -c "printf \"\nhost \t all \t all \t 127.0.0.1/16 \t md5\nhostssl  all \t all \t 127.0.0.1/16 \t md5\n\" >> /etc/postgresql/9.1/main/pg_hba.conf"
printf "\nlisten_addresses = '*'\n" | sudo tee -a /etc/postgresql/9.1/main/postgresql.conf > /dev/null

#Restart PostGres
sudo service postgresql restart

#Create Database User and Table
echo "creating role"
echo "CREATE ROLE $Z_PGUSER WITH CREATEDB LOGIN NOSUPERUSER NOCREATEROLE PASSWORD $PASSWD" | sudo -u postgres psql

echo "create database: $Z_PGDATABASE"
sudo su -c "createdb -O $Z_PGUSER $Z_PGDATABASE" postgres

# Setup the Zest database tables
$PEASOUP_HOME/tools/db/zest/zestdb_create.sh
