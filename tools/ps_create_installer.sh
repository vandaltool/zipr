#!/bin/bash

PEASOUP_APP_WRAPPER_SCRIPT=$1
PEASOUP_APP_BASENAME=`basename $PEASOUP_APP_WRAPPER_SCRIPT`
PEASOUP_APP_PACKAGE=`pwd`/$PEASOUP_APP_BASENAME.peasoup.tar

usage()
{
  echo "usage: $0 <peasoup_program>"
  exit 1
}

verify_peasoup_app()
{
  grep ps_run.sh $PEASOUP_APP_WRAPPER_SCRIPT >/dev/null 2>/dev/null
  if [ ! $? -eq 0 ]; then
    echo "$PEASOUP_APP_WRAPPER_SCRIPT is not a PEASOUP program"
	echo
	usage
  fi
}

#
# Script to package up a Peasoupified binary
#


verify_peasoup_app

rm $PEASOUP_APP_PACKAGE.gz 2>/dev/null

#
# Wrapper script contains a line of the form:
# <fullpath_ps_run.sh> <fullpath_top_level_peasoup_directory> "$@"
#

# Get the peasoup application directory off the 2nd argument on that line
PEASOUP_APP_DIR=`grep ps_run $PEASOUP_APP_WRAPPER_SCRIPT | cut -d' ' -f2`

echo "Peasoup application package  : $PEASOUP_APP_PACKAGE"
echo "Peasoup application directory: $PEASOUP_APP_DIR"

NEW_PEASOUP_DIR=$PEASOUP_APP_BASENAME.peasoup
TMP=/tmp/$PEASOUP_APP_BASENAME.$$
TMP_PEASOUP_DIR=$TMP/$NEW_PEASOUP_DIR
mkdir -p $TMP_PEASOUP_DIR

# copy files necessary to run peasoupified binary
cp $PEASOUP_APP_DIR/a.stratafied $TMP_PEASOUP_DIR
cp $PEASOUP_APP_DIR/a.irdb.bspri $TMP_PEASOUP_DIR
cp $PEASOUP_APP_DIR/a.ncexe.annot $TMP_PEASOUP_DIR
cp $PEASOUP_APP_DIR/ps_run.sh $TMP_PEASOUP_DIR

# fill in values for installer template
INSTALLER=install_$PEASOUP_APP_BASENAME.sh
cp $PEASOUP_HOME/tools/ps_install.stmpl $TMP/$INSTALLER

cat $PEASOUP_HOME/tools/ps_install.stmpl | sed "s/#PEASOUP_DIR#/$NEW_PEASOUP_DIR/g" > $TMP/$INSTALLER

#

# now prepare the zipped tarball
cd $TMP

chmod +x $INSTALLER

tar -cvzf $PEASOUP_APP_PACKAGE \
    $INSTALLER \
    $NEW_PEASOUP_DIR/a.stratafied \
    $NEW_PEASOUP_DIR/a.irdb.bspri \
    $NEW_PEASOUP_DIR/a.ncexe.annot \
    $NEW_PEASOUP_DIR/ps_run.sh 

gzip $PEASOUP_APP_PACKAGE

# cleanup and restore working directory
rm -fr $TMP

echo
echo "Created zipped tarball for peasoup binary: $PEASOUP_APP_WRAPPER_SCRIPT"
echo "To install, unzip tarball and run installer script" 
