#!/bin/bash

# This script automatically sets up the environment

# add the following lines to .bashrc so that subsequent terminals opened will contain the right ENV VAR settings
echo "# Adding lines to set up environment variables for CGC DEV" >> $HOME/.bashrc
echo "cd $HOME/peasoup_umbrella" >> $HOME/.bashrc
echo ". ./set_env_vars" >> $HOME/.bashrc
echo "cd .." >> $HOME/.bashrc

# set up the environment for this terminal
. ./set_env_vars

# get the peasoup packages
./get-peasoup-packages.sh all 2>&1 | tee get_packages.output

# build the packages
./build-all.sh 2>&1 | tee build.output

# set up the irdb
./postgres_setup.sh 2>&1 | tee pgsetup.output

