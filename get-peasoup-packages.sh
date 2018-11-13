#!/bin/bash

#
# in case your OS doesn't support i386 packages.
#
#dpkg --add-architecture i386
#sudo apt-get update

# Needed to build components
BASE_PKGS="
  scons
  dwarfdump
  bison
  flex
  g++
  nasm
  openjdk-6-jre
  sharutils
  subversion
  xdotool
  gcc-multilib
  g++-multilib
  autoconf
  realpath
  libelf-dev
  libxqilla-dev
  libxerces-c-dev
  screen
  libxml2-dev
  libstdc++6:i386
  coreutils
  makeself"
# TODO: don't require i386 libraries if not running MEDS (eg using IDA server)

# For clients of IRDB
CLIENT_IRDB_PKGS="
  postgresql-client
  pgadmin3
  libpqxx-dev
  libmysqlclient-dev
  scons
  cmake
  automake1.9"

# For IRDB server
SERVER_IRDB_PKGS="postgresql"

ALL_PKGS="$BASE_PKGS $CLIENT_IRDB_PKGS $SERVER_IRDB_PKGS "


install_packs()
{
	for i in $*
	do
		which apt-get 1> /dev/null 2> /dev/null 
		if [[ $? == 0  ]]; then
			sudo apt-get install $i -y 
		else 
			sudo yum install $i -y
		fi
	done
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

echo Intsalling packages complete.
