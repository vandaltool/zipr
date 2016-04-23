#!/bin/bash -x
#
# Copyright (c) 2014 - Zephyr Software LLC
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
#
# This software was developed with SBIR funding and is subject to SBIR Data Rights, 
# as detailed below.
#
# SBIR DATA RIGHTS
#
# Contract No. __N00014-14-C-0197___W31P4Q-14-C-0086________.
# Contractor Name __Zephyr Software LLC_____________________.
# Address __2040 Tremont Road, Charlottesville, VA 22911____.
# Expiration of SBIR Data Rights Period __16-JUNE-2021______.
#

ANALYSIS_DIR=$1  # fully-qualified path for peasoup_executable_directory_XXX
PROG=$2          # cat.protected

USER_DOWNLOAD_DIR=/tmp/zest_protected/download/$$

INSTALLER_SCRIPT=$PROG.install.sh

# tarball format:
#   cat.installer/cat.install.sh
#   cat.installer/peasoup_executable_directory_XXX
ANALYSIS_LOCAL_DIR=$(dirname $ANALYSIS_DIR)
INSTALLER_DIR=$PROG.installer

mkdir -p $USER_DOWNLOAD_DIR/$INSTALLER_DIR 2>/dev/null

# scrub the peasoup directory before making a tarball
$PEASOUP_HOME/tools/ps_scrub.sh $ANALYSIS_DIR

# get rid of any previous tarballs
cd $USER_DOWNLOAD_DIR/$INSTALLER_DIR
rm -fr *.tar *.tgz *peasoup* *instal* *analysis*

# make install script
cp $PEASOUP_HOME/tools/ps_install.sh $PROG.install.sh
chmod +x $PROG.install.sh
# link to analysis dir
ln -s $ANALYSIS_DIR analysis_dir

# make the tarball
cd $USER_DOWNLOAD_DIR
tar -hcvf $PROG.tar $INSTALLER_DIR
gzip $PROG.tar
mv $PROG.tar.gz "${ANALYSIS_DIR}/${PROG}.tgz"

# cleanup
#if [ ! -z $USER_DOWNLOAD_DIR ]; then
#	rm -fr "$USER_DOWNLOAD_DIR"
#fi

