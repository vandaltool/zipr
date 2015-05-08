#!/bin/bash -x

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

# Needed for afl/qemu mode
AFL_PKGS="wget libglib2.0-dev"

ALL_PKGS="$BASE_PKGS $CLIENT_IRDB_PKGS $SERVER_IRDB_PKGS $TEST_PKGS $SQL_PKGS $LDAP_PKGS $AFL_PKGS"


if [[ "$PEASOUP_UMBRELLA_DIR" == "" ]]; then
	echo "PEASOUP_UMBRELLA_DIR not found"
	echo "Did you source set_env_vars and use sudo -E"
	exit 1
fi

function install_afl {
        current_dir=`pwd`
	afl_dir=${current_dir}/afl_download.$$

	# get the latest and greatest afl
	wget http://lcamtuf.coredump.cx/afl/releases/afl-latest.tgz
	
	mkdir $afl_dir
	tar -xzvf afl-latest.tgz -C $afl_dir

	# build afl
	cd $afl_dir/afl*
	make

	# build qemu support
	cd qemu_mode
	./build_qemu_support.sh

	# install on the system
	cd ..
	sudo make install

	# installed successfully?
	afl-fuzz | grep README
	if [ $? -eq 0 ]; then
		echo "#######################################################"
		echo "AFL has been successfully installed"
		echo "#######################################################"
		rm ${current_dir}/afl-latest.tgz
		rm -fr $afl_dir
	else
		echo "Something went wrong with the AFL installation"
	fi

	cd $current_dir
}

for arg in $@; do
    case $arg in
    all)
	sudo apt-get -y install $ALL_PKGS
	install_afl
	;;
    afl)
	sudo apt-get -y install $AFL_PKGS
	install_afl
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



