#!/bin/bash 
#
# ps_analyze.sh - analyze a program and transform it for peasoupification to prevent exploit.
#
# This script depends on having many environment variables defined, but it should check that they are defined properly for you.
#
# Usage:
#     peasoup_analyze.sh <original_binary> <new_binary> <options>
#


#
# check that the remaining options are validly parsable, and record what they are.
#
check_options()
{

	# 
	# fill in better option parsing later.
	#
	if [ ! -z $1 ]; then
		echo Cannot parse option $1
		exit 1
	fi
	
}


#
# subroutine to determine if a particular phase of ps_analyze is on.
#
is_step_on()
{
	# for now, all steps are on 
	return 1
}

#
# get_step_error_code
#
is_step_error()
{
	#
	# fill in with better info later
	#
	return 0
}

#
# Detect if this step of the computation is on, and execute it.
#
perform_step()
{
	step=$1
	shift
	command="$*"

	is_step_on $step
	if [ $? -eq 0 ]; then 
		return 0
	fi

	logfile=logs/$step.log

	echo -n Performing step "$step" ...
	starttime=`date`

	# If verbose is on, tee to a file 
	if [ ! -z "$VERBOSE" ]; then
		$command 2>&1 | tee $logfile
		command_exit=${PIPESTATUS[0]} # this funkiness gets the exit code of $command, not tee
	else
		$command > $logfile 2>&1 
		command_exit=$?
	fi
	
	echo command exit status for step $step is $command_exit

	is_step_error $step $command_exit
	if [ $? -ne 0 ]; then
		echo command failed!
	else
		echo Done.  Successful.
	fi

	echo "# attribute start_time=$starttime" >> $logfile
	echo "# attribute end_time=`date`" >> $logfile
	echo "# attribute peasoup_step_name=$step" >> $logfile
	echo "# attribute peasoup_step_number=$stepnum" >> $logfile
	echo "# attribute peasoup_step_exitcode=$command_exit" >> $logfile

	# move to the next step 
	stepnum=`expr $stepnum + 1`

	all_logs="$all_logs $logfile"

	return $command_exit
}


#
# create a log for ps_analyze
#
report_logs()
{
	logfile=logs/ps_analyze.log

	echo "# attribute start_time=$ps_starttime" >> $logfile
	echo "# attribute end_time=`date`" >> $logfile
	echo "# attribute peasoup_step_name=all_peasoup" >> $logfile

	for i in $all_logs
	do
		echo >> $logfile
		echo ------------------------------------------------------- >> $logfile
		echo ----- From $i ------------------- >> $logfile
		echo ------------------------------------------------------- >> $logfile
		cat $i |sed "s/^# attribute */# attribute renamed_for_ps/" >> $logfile
		echo ------------------------------------------------------- >> $logfile
		echo >> $logfile
	done
}



#
# check if the list of environment variables passed are all defined.
#
check_environ_vars()
{

	while [ true ]; 
	do

		# done?
		if [ -z $1 ]; then
			return;
		fi

        	# create the $ENVNAME string in varg
        	varg="\$$1"

        	# find out the environment variable's setting
        	eval val=$varg

		if [ -z $val ]; then echo Please set $1; exit 1; fi

		shift 
	done

}

#
# Check that the filenames passed are valid.
#
check_files()
{

	while [ true ]; 
	do

		# done?
		if [ -z $1 ]; then
			return;
		fi

		if [ ! -f $1 ]; then 
			echo PEASOUP ERROR:  $1  not found.  Is there an environment var set incorrectly?
			exit 1; 
		fi

		shift 
	done

}


ps_starttime=`date`

#
# turn on debugging output if it's requested.
#
if [ ! -z "$VERBOSE" ]; then
	set -x
fi


#
# stepnum used for counting how many steps peasoup executes
# 
stepnum=0


#
# Check for proper environment variables and files that are necessary to peasoupify a program.
#
check_environ_vars PEASOUP_HOME SMPSA_HOME STRATA_HOME SECURITY_TRANSFORMS_HOME IDAROOT
check_files $PEASOUP_HOME/tools/getsyms.sh $SMPSA_HOME/SMP-analyze.sh  $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh 


#
# Check/parse options
#
if [ -z $2 ]; then
  echo "Usage: $0 <original_binary> <new_binary> <options>"
  exit 1
fi

#
# record the original program's name
#
orig_exe=$1
newname=a
shift

#
# record the new program's name
#
stratafied_exe=$1
shift

#
# finish argument parsing
#
check_options $*


#
# new program
#
name=`basename $orig_exe`
newdir=peasoup_executable_directory.$name.$$

# create a working dir for all our files using the pid
mkdir $newdir

# store the original executable as a.ncexe
cp $orig_exe $newdir/$newname.ncexe

# make sure we overwrite out output file one way or another
rm -f $stratafied_exe

# and switch to that dir
cd $newdir

# next, create a location for our log files
mkdir logs 	

#
# create a stratafied binary that does pc confinement.
#
perform_step stratafy_with_pc_confine sh $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh $newname.ncexe $newname.stratafied 

#
# Let's output the modified binary
# This binary will really be a shell script that calls the newly stratafied binary
#
perform_step create_binary_script $PEASOUP_HOME/tools/do_makepeasoupbinary.sh $name 


#
# Running IDA Pro static analysis phase ...
#
perform_step meds_static $PEASOUP_HOME/tools/do_idapro.sh

#
# Run concolic engine
#
perform_step concolic $PEASOUP_HOME/tools/do_concolic.sh a  -t 600 -u 60 -i 25 -l tracer,trace,inputs  > do_concolic.out 2>&1


##
## Populate IR Database
##

#
# get some simple info for the program
#	
DB_PROGRAM_NAME=`basename $orig_exe.$$ | sed "s/[\.;+\\-\ ]/_/g"`
MD5HASH=`md5sum $newname.ncexe | cut -f1 -d' '`

#
# register the program
#
perform_step pdb_register "$PEASOUP_HOME/tools/db/pdb_register.sh $DB_PROGRAM_NAME `pwd`"
varid=$?


#
# create the tables for the program
#
perform_step pdb_create_tables $PEASOUP_HOME/tools/db/pdb_create_program_tables.sh $DB_PROGRAM_NAME  

#
# check to see if annot file exists before doing anything, and also that we've created a variant.
#
if [ -f $newname.ncexe.annot  -a $varid -gt 0 ]; then

	# import meds info to table
	perform_step meds2pdb $SECURITY_TRANSFORMS_HOME/tools/meds2pdb/meds2pdb $DB_PROGRAM_NAME $newname.ncexe $MD5HASH $newname.ncexe.annot 	 

	# build basic IR
	perform_step fill_in_cfg $SECURITY_TRANSFORMS_HOME/libIRDB/test/fill_in_cfg.exe $varid	
	perform_step fill_in_indtargs $SECURITY_TRANSFORMS_HOME/libIRDB/test/fill_in_indtargs.exe $varid ./$newname.ncexe    

	# finally create a clone so we can do some transforms 
	perform_step clone $SECURITY_TRANSFORMS_HOME/libIRDB/test/clone.exe $varid				
	cloneid=$?

	if [ $cloneid -gt 0 ]; then
		# do the basic tranforms we're performing for peasoup 
		perform_step fix_calls $SECURITY_TRANSFORMS_HOME/libIRDB/test/fix_calls.exe $cloneid	
#		perform_step cover $PEASOUP_HOME/tools/cover.sh 
		perform_step p1transform $PEASOUP_HOME/tools/do_p1transform.sh $cloneid $newname.ncexe $newname.ncexe.annot
		perform_step integertransform $PEASOUP_HOME/tools/do_integertransform.sh $cloneid
		perform_step ilr $SECURITY_TRANSFORMS_HOME/libIRDB/test/ilr.exe $cloneid 

		# generate aspri, and assemble it to bspri
		perform_step generate_spri $SECURITY_TRANSFORMS_HOME/libIRDB/test/generate_spri.exe $cloneid a.irdb.aspri
		perform_step spasm $SECURITY_TRANSFORMS_HOME/tools/spasm/spasm a.irdb.aspri a.irdb.bspri stratafier.o.exe 
	fi
fi

#
# create a report for all of ps_analyze.
#
report_logs

# go back to original directory
cd - > /dev/null 2>&1

cp $newdir/$name.sh $stratafied_exe


# return the exit code of the copy as the final return value 
# So that a predictable return value is returned
retval=$?
exit $retval
