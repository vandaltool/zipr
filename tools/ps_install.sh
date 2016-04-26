#!/bin/bash

# Installer name is of the form: program.install.sh
PROG=$(basename $0 .installer.sh)

# Default installation directory
INSTALL_DIR=/tmp/zest_install/bin
DESKTOP_ZEPHYR_DIR=$HOME/Desktop/ZephyrProtected

create_desktop_shortcut()
{
	name=$1
	pathprotected=$2
	if [ -d "$HOME/Desktop" ]; then
		if [ ! -d "$DESKTOP_ZEPHYR_DIR" ]; then
			mkdir $DESKTOP_ZEPHYR_DIR
		fi
		desktopfile=${DESKTOP_ZEPHYR_DIR}/${name}.desktop
		echo "[Desktop Entry]" > $desktopfile
		echo "Name=$name" >> $desktopfile
		echo "Comment=Run $name" >> $desktopfile
		echo "Exec=${pathprotected}" >> $desktopfile
		echo 'Terminal=false' >> $desktopfile
		echo 'Type=Application' >> $desktopfile
		echo 'Icon=utilities-terminal' >> $desktopfile
		chmod +x $desktopfile
		return 0
	else
		return 1
	fi
}

echo ================================================================
echo "Installing $PROG"
echo "Installation directory: $INSTALL_DIR"
echo "Fully qualified path: $INSTALL_DIR/$PROG"

echo
echo "Type any key to continue"
read anykey

# in case it's not there yet
if [ ! -d "$INSTALL_DIR" ]; then
	mkdir -p $INSTALL_DIR 
fi

chmod +x ./a.stratafied

# wipe out any old installs
if [ -d "$INSTALL_DIR/${PROG}_analysis" ]; then
	rm -fr "$INSTALL_DIR/${PROG}_analysis"
fi

# copy the peasoup dir
cp -r . $INSTALL_DIR/${PROG}_analysis
echo "$INSTALL_DIR/${PROG}_analysis/ps_run.sh $INSTALL_DIR/${PROG}_analysis \"\$0\" \"\$@\"" > $INSTALL_DIR/$PROG
chmod +x $INSTALL_DIR/$PROG

# if X11 applications detected, create desktop shortcut
ldd ./a.ncexe.orig | grep X11 >/dev/null 2>&1
if [ $? -eq 0 ]; then
	create_desktop_shortcut ${PROG} ${INSTALL_DIR}/${PROG}
	if [ $? -eq 0 ]; then
		echo X application detected - desktop shortcut succesfully created in $DESKTOP_ZEPHYR_DIR
	else
		echo Could not create desktop shortcut in $DESKTOP_ZEPHYR_DIR
	fi
fi

echo ================================================================
echo "Installation complete"
echo ================================================================

# cleanup 

#
# note that we are in the archive
#

# save current dir
current_dir=$(pwd)

# remove files in the current directory
rm -r * 

# get out of current dir so that we can erase it
cd /tmp
rmdir "${current_dir}"

# remove temporary installer
rm -f /tmp/${PROG}.installer

