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


USER=$1          # not used yet
PROG=$2          # cat
JOBID=$3         # 1324
ANALYSIS_DIR=$4  # fully-qualified path for peasoup_executable_directory_XXX

USER_DOWNLOAD_DIR=/tmp/zest/$USER/download

INSTALLER_SCRIPT=$PROG.install.sh

INSTALLER="/download/$JOBID.tgz"

# scrub the peasoup directory before making a tarball
$PEASOUP_HOME/tools/ps_scrub.sh $ANALYSIS_DIR

# tarball format:
#   cat.installer/cat.install.sh
#   cat.installer/peasoup_executable_directory_XXX
ANALYSIS_LOCAL_DIR=$(dirname $ANALYSIS_DIR)
INSTALLER_DIR=$PROG.installer

mkdir -p $USER_DOWNLOAD_DIR/$INSTALLER_DIR 2>/dev/null

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
tar -hcvf $JOBID.tar $INSTALLER_DIR
gzip $JOBID.tar
mv $JOBID.tar.gz $JOBID.tgz

# update the database
$PEASOUP_HOME/tools/db/job_spec_update_installer.sh $JOBID $INSTALLER
