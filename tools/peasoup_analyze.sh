#!/bin/sh
# This script depends on having the following environment variables defined
# STRATA - The path to the strata installation
# An example of these environment variables and their settings are listed in
# the sample file: $STRATA/security_startup_rc
#
# Usage:
#     peasoup_analyze.sh <original_binary> [ <new_binary> ] 
#
# Version 1 - prepares binary for PC confinement


if [ "$PEASOUP_HOME"X = X ]; then echo Please set PEASOUP_HOME; exit 1; fi
if [ ! -f  $PEASOUP_HOME/tools/getsyms.sh ]; then echo PEASOUP_HOME is set poorly, please fix.; exit 1; fi
if [ "$SMPSA_HOME"X = X ]; then echo Please set SMPSA_HOME; exit 1; fi
if [ ! -f  $SMPSA_HOME/SMP-analyze.sh ]; then echo SMPSA_HOME is set poorly, please fix.; exit 1; fi
if [ "$STRATA_HOME"X = X ]; then echo Please set STRATA_HOME; exit 1; fi
if [ ! -f  $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh ]; then echo STRATA_HOME is set poorly, please fix.; exit 1; fi

if [ -z $2 ]; then
  echo "Usage: $0 <original_binary> <new_binary>"
  exit 1
fi

orig_exe=$1

if [ -z $2 ]; then
stratafied_exe=$orig_exe
else
stratafied_exe=$2
fi

echo "Original program: $orig_exe   New program: $stratafied_exe"

name=`basename $orig_exe`
newdir=$name.$$

echo "Switching to directory $newdir"

mkdir $newdir
cp $orig_exe $newdir/$name.orig
cd $newdir

sh $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh $name.orig $name.stratafied

# We've now got a stratafied program

# Let's output the modified binary
# This binary will really be a shell script that calls the newly stratafied binary


current_dir=`pwd`
peasoup_binary=$name.peasoup

echo "#!/bin/sh" >> $peasoup_binary
echo "" >> $peasoup_binary
echo "#for some reason, pc confinment is broker"
echo "#STRATA_PC_CONFINE=1 $current_dir/$name.stratafied \$*" >> $peasoup_binary
echo "$current_dir/$name.stratafied \$*" >> $peasoup_binary

chmod +x $peasoup_binary

cd -

cp $newdir/$name.peasoup $stratafied_exe
