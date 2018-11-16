#!/bin/bash

#
# in case your OS doesn't support i386 packages.
#
#dpkg --add-architecture i386
#sudo apt-get update

# Needed to build components
BASE_PKGS="
  scons
  bison
  flex
  g++
  nasm
  openjdk-6-jre
  sharutils
  gcc-multilib
  g++-multilib
  autoconf
  realpath
  apt-libelf-dev
  yum-libelf-devel
  apt-libxqilla-dev
  yum-libxqilla-devel
  apt-libxerces-c-dev
  yum-libxerces-c-devel
  apt-libxml2-dev
  yum-libxml2-devel
  libstdc++6:i386
  coreutils
  makeself"
# TODO: don't require i386 libraries if not running MEDS (eg using IDA server)

# For clients of IRDB
CLIENT_IRDB_PKGS="
  postgresql-client
  yum-postgresql-server 
  yum-postgresql-contrib
  pgadmin3
  apt-libpqxx-dev
  yum-libpqxx-devel
  scons
  cmake
  automake1.9"

# For IRDB server
SERVER_IRDB_PKGS="
	postgresql
"

ALL_PKGS="$BASE_PKGS $CLIENT_IRDB_PKGS $SERVER_IRDB_PKGS "


install_packs()
{
	for i in $*
	do
		which apt-get 1> /dev/null 2> /dev/null 
		if [[ $? == 0  ]]; then
			if [[ $i =~ apt-* ]]; then
				apters="$apters $(echo $i|sed "s/^apt-//")"
			elif [[ $i =~ yum-* ]]; then
				echo "Skipping install of $i for platform  $(lsb_release -d -s)"
			else
				apters="$apters $i"
			fi
		else 
			if [[ $i =~ apt-* ]]; then
				echo "Skipping install of $i for platform  $(cat /etc/redhat-release)"
			elif [[ $i =~ yum-* ]]; then
				yummers="$yummers $(echo $i|sed "s/^yum-//")"
			else
				yummers="$yummers $i"
			fi
		fi
	done
	which apt-get 1> /dev/null 2> /dev/null 
	if [[ $? == 0  ]]; then
		sudo apt-get install -y --ignore-missing $apters
	else
		sudo yum install -y --skip-broken $yummers
	fi
}

args="$@"
if [[ $args = "" ]]; then
	args="all"
fi

for arg in $args; do
    case $arg in
    all)
	install_packs $ALL_PKGS
	;;
    base)
	install_packs $BASE_PKGS
	;;
    client-irdb)
	install_packs $CLIENT_IRDB_PKGS
	;;
    server-irdb)
	install_packs $SERVER_IRDB_PKGS
	;;
    irdb)
	install_packs $CLIENT_IRDB_PKGS $SERVER_IRDB_PKGS
	;;
    *)
	echo "$arg not recognized. Recognized args: all, base, client-irdb,";
	echo "  server-irdb, irdb, test, sql.";
    esac
done


orig_dir=$(pwd)
echo "Getting irdb_transforms packages."
cd irdb_transforms
sudo ./get-packages.sh
cd $orig_dir

cd daffy
sudo ./get-packages.sh
cd $orig_dir

echo Intsalling packages complete.
