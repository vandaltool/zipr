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
# Version 2 - runs Grace
# Version 3 - runs p1 transform


if [ "$PEASOUP_HOME"X = X ]; then echo Please set PEASOUP_HOME; exit 1; fi
if [ ! -f  $PEASOUP_HOME/tools/getsyms.sh ]; then echo PEASOUP_HOME is set poorly, please fix.; exit 1; fi
if [ "$SMPSA_HOME"X = X ]; then echo Please set SMPSA_HOME; exit 1; fi
if [ ! -f  $SMPSA_HOME/SMP-analyze.sh ]; then echo SMPSA_HOME is set poorly, please fix.; exit 1; fi
if [ "$STRATA_HOME"X = X ]; then echo Please set STRATA_HOME; exit 1; fi
if [ ! -f  $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh ]; then echo STRATA_HOME is set poorly, please fix.; exit 1; fi
# mc2zk commenting this line out temporarily until SECURITY_TRANSFORMS development is merged to the trunk
#if [ "$SECURITY_TRANSFORMS_HOME"X = X ]; then echo Please set SECURITY_TRANSFORMS; exit 1; fi

if [ -z $2 ]; then
  echo "Usage: $0 <original_binary> <new_binary>"
  exit 1
fi

orig_exe=$1
newname=a

if [ -z $2 ]; then
stratafied_exe=$orig_exe
else
stratafied_exe=$2
fi

date
echo "Original program: $orig_exe   New program: $stratafied_exe"

name=`basename $orig_exe`
newdir=peasoup_executable_directory.$name.$$

mkdir $newdir
cp $orig_exe $newdir/$newname.ncexe
cd $newdir


echo -n Creating stratafied executable...
sh $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh $newname.ncexe $newname.stratafied > /dev/null 2>&1 
echo Done. 

# We've now got a stratafied program

# Let's output the modified binary
# This binary will really be a shell script that calls the newly stratafied binary

current_dir=`pwd`
peasoup_binary=$name.sh

echo "#!/bin/sh" >> $peasoup_binary
echo "" >> $peasoup_binary
echo "$PEASOUP_HOME/tools/ps_run.sh $current_dir \"\$@\"" >> $peasoup_binary 



chmod +x $peasoup_binary


echo Running IDA Pro static analysis phase ...
$SMPSA_HOME/SMP-analyze.sh a.ncexe
echo Done.

echo Running concolic testing to generate inputs ...
#$PEASOUP_HOME/tools/do_concolic.sh a  --iterations 25 --logging tracer,instance_times,trace
$PEASOUP_HOME/tools/do_concolic.sh a  --iterations 25 --logging tracer,trace,inputs 
# 2>&1 |egrep -e "INPUT VECTOR:" -e "1: argc ="
# >/dev/null 2>&1 
echo Done.


#
# Uncomment this part to test the P1 xform
#

#-----------------------------------------
# Start P1 transform 
#-----------------------------------------
#
#echo Starting the P1 transform
#date
#$PEASOUP_HOME/tools/p1xform.sh $newdir > p1xform.out 2> p1xform.err
#
#echo $current_dir/$newdir/p1.xform/p1.final
#
#if [ -f $current_dir/p1.xform/p1.final ]; then
#  echo List of functions transformed:
#  cat $current_dir/p1.xform/p1.final
#else
#  echo P1 was unable to transform the subject program
#fi
#
#date
#echo Done with the P1 transform

#-----------------------------------------
# End P1 transform 
#-----------------------------------------

# go back to original directory
cd - > /dev/null 2>&1

cp $newdir/$name.sh $stratafied_exe

# return the exit code of the copy as the final return value 
# So that a predictable return value is returned
retval=$?
exit $retval
