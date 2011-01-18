#!/bin/sh

# This script demonstrates the steps for ps_analyze.sh on a specific directory
# It exists merely to save some time in live demonstrating some of the steps by
# making use of some existing intermediate products of the ps_analyze process.
#
# If a rebuild is necessary, some of the values in this script must be changed.
# They are annotated below.


# Save the current working directory
START_DIR=`pwd`

# name of the target exe
name=dumbledore_cmd.original

# CHANGE this directory to the newly built dumbledore_cmd directory
# Should be of the form : peasoup_executable_directory.dumbledore_cmd.original.<pid>
# This speeds up the IDA Pro step because intermediate files (particularly idb) are already in existence
EXE_DIR=${PEASOUP_HOME}/demos/demo.jan_21_2011/peasoup_executable_directory.dumbledore_cmd.original.2051
 
cd ${EXE_DIR}

# Perform Stratafication
echo -n Creating stratafied executable...
sh $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh a.ncexe a.stratafied > /dev/null 2>&1
echo Stratafication completed.

current_dir=`pwd`
peasoup_binary=$name.sh

echo "#!/bin/sh" >> $peasoup_binary
echo "" >> $peasoup_binary
echo "$PEASOUP_HOME/tools/ps_run.sh $current_dir \$*" >> $peasoup_binary
chmod +x $peasoup_binary

# Run the IDA Pro static analysis phase
echo Running IDA Pro static analysis phase ...
$SMPSA_HOME/SMP-analyze.sh a.ncexe
echo Static analysis phase completed.

# Run GraCE
echo Running GraCE concolic testing to generate inputs ...
$PEASOUP_HOME/tools/do_concolic.sh a --iterations 40 2>&1 |egrep -e "INPUT VECTOR:" -e "1: argc ="
echo GraCE concolic testing phase complete.

cd - > /dev/null 2>&1

cp ${EXE_DIR}/${name}.sh dumbledore_cmd.protected

