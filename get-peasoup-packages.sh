#!/bin/bash

# Needed to build PEASOUP components
# removing libelf for cgc as it causes a conflict  libelf-dev
BASE_PKGS="
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
  realpath
  libxqilla-dev
  libxerces-c-dev
  screen
  libxml2-dev
  libstdc++6:i386
  coreutils"
# TODO: don't require i386 libraries if not running MEDS (eg using IDA server)

# For clients of IRDB
CLIENT_IRDB_PKGS="
  postgresql-client
  pgadmin3
  libpqxx3-dev
  libmysqlclient-dev
  cmake
  automake1.9"

# For IRDB server
SERVER_IRDB_PKGS="postgresql"

# For building test subjects
TEST_PKGS="
  asciidoc
  autoconf
  bison
  gawk
  gettext
  libx11-dev
  libfontconfig1-dev
  libperl-dev
  poedit
  yasm
  xvfb"

# For handling SQL command injections
SQL_PKGS="sqlite3 libsqlite3-dev mysql-client mysql-server libmysqlclient-dev"

# For LDAP
LDAP_PKGS="ldap-utils slapd libldap2-dev"

ALL_PKGS="$BASE_PKGS $CLIENT_IRDB_PKGS $SERVER_IRDB_PKGS $TEST_PKGS $SQL_PKGS $LDAP_PKGS"


if [[ "$PEASOUP_UMBRELLA_DIR" == "" ]]; then
	echo "PEASOUP_UMBRELLA_DIR not found"
	echo "Did you source set_env_vars and use sudo -E"
	exit 1
fi

for arg in $@; do
    case $arg in
    all)
	sudo apt-get -y install $ALL_PKGS
	;;
    base)
	sudo apt-get -y install $BASE_PKGS
	;;
    client-irdb)
	sudo apt-get -y install $CLIENT_IRDB_PKGS
	;;
    server-irdb)
	sudo apt-get -y install $SERVER_IRDB_PKGS
	;;
    irdb)
	sudo apt-get -y install $CLIENT_IRDB_PKGS $SERVER_IRDB_PKGS
	;;
    test)
	sudo apt-get -y install $TEST_PKGS
	;;
	sql)
	sudo apt-get -y install $SQL_PKGS
	;;
	ldap)
	sudo apt-get -y install $LDAP_PKGS
	;;
    *)
	echo "$arg not recognized. Recognized args: all, base, client-irdb,";
	echo "  server-irdb, irdb, test, sql.";
    esac
done
