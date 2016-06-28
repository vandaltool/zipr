#!/bin/bash

#Add PostGres password file for modifying the database
printf ":5432::$USER:1qaz2wsx\nlocalhost:5432:*:$USER:1qaz2wsx\n127.0.0.1:5432:*:$USER:1qaz2wsx" > $HOME/.pgpass
chmod og-rw $HOME/.pgpass

#Allow remote access to PostGres
sudo su -c "printf \"\nhost \t all \t all \t 127.0.0.1/16 \t md5\nhostnossl  all \t all \t 127.0.0.1/16 \t md5\n\" >> /etc/postgresql/9.4/main/pg_hba.conf"
printf "\nlisten_addresses = '*'\n" | sudo tee -a /etc/postgresql/9.4/main/postgresql.conf > /dev/null

#Restart PostGres
sudo service postgresql restart

#Create Database User and Table
echo "CREATE ROLE $USER WITH CREATEDB LOGIN NOSUPERUSER NOCREATEROLE PASSWORD '1qaz2wsx'" | sudo -u postgres psql
sudo su -c "createdb -O $USER peasoup_$USER" postgres

#Setup the Database to store PEASOUP info
source set_env_vars
$PEASOUP_HOME/tools/db/pdb_setup.sh
