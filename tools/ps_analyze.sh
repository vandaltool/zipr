#!/bin/bash 
#
# ps_analyze.sh - analyze a program and transform it for peasoupification to prevent exploit.
#
# This script depends on having many environment variables defined, but it should check that they are defined properly for you.
#
# Usage:
#     peasoup_analyze.sh <original_binary> <new_binary> <options>
#

source $(dirname $0)/ps_wrapper.source $0

realpath() {
  \cd "$1"
  /bin/pwd
}


##################################################################################
# set default values for 
##################################################################################

initial_on_phases="stratafy_with_pc_confine create_binary_script is_so gather_libraries meds_static pdb_register fill_in_cfg fill_in_indtargs clone fix_calls generate_spri spasm fast_annot fast_spri preLoaded_ILR1 preLoaded_ILR2"

##################################################################################

ulimit -s unlimited > /dev/null 2>&1 || true

# default watchdog value is 30 seconds
#watchdog_val=30
errors=0
warnings=0

# record statistics in database?
record_stats=0

# DEFAULT TIMEOUT VALUE
INTEGER_TRANSFORM_TIMEOUT_VALUE=1800
TWITCHER_TRANSFORM_TIMEOUT_VALUE=1800
# Setting PN timeout to 6 hours for TNE. 
# PN_TIMEOUT_VALUE=21600

export backend=strata

# 
# set default values for 
#

CONCOLIC_DIR=concolic.files_a.stratafied_0001

# JOBID

JOBID="$(basename $1).$$"

# 
# By default, big data approach is off
# To turn on the big data approach: modify check_options()
#

# alarm handler
THIS_PID=$$
handle_alarm()
{
	# reset handler
	trap - ALRM

	#
	# create a report for all of ps_analyze.
	#
	report_logs

	# go back to original directory
	cd - > /dev/null 2>&1

	# stop ps_analyze
	kill -9 $THIS_PID

	# exit timer process: SIGALRM + 128
	exit 142
}

set_timer()
{
	# set handler
	trap "handle_alarm" ALRM

	# wait
	sleep $1& wait

	# signal the alarm
	kill -ALRM $$
}

fail_gracefully()
{
	if [ ! -z $TIMER_PID ]; then
		kill -9 $TIMER_PID
	fi
	echo $1
	echo
	# display usage too.
	usage
	exit 255
}

adjust_lib_path()
{
	NEWPATH=
	for i in `echo $LD_LIBRARY_PATH | sed 's/:/ /g'`
	do
		alp_newdir=`realpath $i	 2> /dev/null`
		if [ $? = 0  ] ; then 
			NEWPATH=$NEWPATH:$alp_newdir
		fi
	done


	# also, add newdir to the ld-library path for analysis.
	export LD_LIBRARY_PATH=$NEWPATH:$PWD/$newdir
}

check_step_option()
{
	echo $1|egrep "=off$|=on$" > /dev/null
	if [ $? -ne 0 ]; then
		echo Malformed option: $1;
		exit -4;
	fi
	
}


set_step_option()
{
	step=`echo "$1" | cut -d: -f1` 
	option=`echo "$1" | cut -s -d: -f2-` 

	# echo "Found step-option for '$step':'$option'"
	if [[ -z "$option" ]]; then
		echo "Cannot parse step:option pair out of '$1'"
		exit 2
	fi

	#
	# this sets step_options_$step to have the new option
	# you can now, when writing your step, just add $step_options_<stepname> where you want the options passed to your step.
	#
	var="step_options_$step"
	old_value="${!var}"
	new_value="$old_value $option"
	eval "step_options_$step=\"$new_value\""
}

usage()
{
	echo "Protect an input program, generating a new executable."
	echo "ps_analyze.sh <input> <output> <options>  "
	echo 
	echo "Where options can be any of"
	echo "   --step <stepname>=(on|off) 		Turn the <stepname> step on or off"
	echo "   -s <stepname>=(on|off)			Same as --step"
	echo "   --step-option <stepname>:<option>	Pass additional option to step <stepname>"
	echo "   -o <stepname>:<option>			Same as --step-option"
	echo "   --timeout				Specify a timeout for ps_analyze.sh."
	echo "   -t					Same as --timeout"
	echo "   --watchdog				Specify a watchdog timer for the protected program."
	echo "   -w					Same as --watchdog"
	echo "   --help					Print this page."
	echo "   --usage				Same as --help"
	echo "   --id <jobid>				Unsupported.  Ask an7s."
	echo "   --name <dbname>			Unsupported.  Ask an7s."
	echo "   --manual_test_script <scriptname>	Specify how to test to the program.  API documentation incomplete."
	echo "   --manual_test_coverage_file <file>	Specify a profile for the program.  API documentation incomplete."
	echo "   --tempdir <dir>			Specify where the temporary analysis files are stored, default is peasoup_executable_directory.<exe>.<pid>"
	echo "   --backend <zipr|strata>		Specify the backend rewriting technology to use.  Default: Strata"
	echo "   -b <zipr|strata>			same as --backend "
	echo "   --stop-after <step>			Stop ps_analyze after completeling the specified step."
	echo "   --stop-before <step>			Stop ps_analyze before starting the specified step."
	echo "   --dump-after <step>			Dump IR after completeling the specified step."
	echo "   --dump-before <step>			Dump IR before starting the specified step."

}



#
# check that the remaining options are validly parsable, and record what they are.
#
check_options()
{

	#
	# turn on initial default set of phases
	#
	for phase in $initial_on_phases
	do
		echo $phases_spec|egrep "$phase=" > /dev/null
		if [ ! $? -eq 0 ];
		then
			phases_spec="$phases_spec $phase=on"
		fi
	done

	# 
	# loop to process options.
	# 

	# Note that we use `"$@"' to let each command-line parameter expand to a 
	# separate word. The quotes around `$@' are essential!
	# We need TEMP as the `eval set --' would nuke the return value of getopt.
	short_opts="s:t:w:b:o:h"
	long_opts="--long step-option: 
		   --long step: 
		   --long timeout: 
		   --long id:  				
		   --long name:	  			
		   --long manual_test_script: 
		   --long manual_test_coverage_file: 
		   --long watchdog: 
		   --long backend:  			
		   --long tempdir:  			
		   --long help
		   --long usage
		   --long stop-after:
		   --long stop-before:
		   --long dump-after:
		   --long dump-before:
		"

	# solaris does not support long option names
	if [ `uname -s` = "SunOS" ]; then
		TEMP=`getopt $short_opts "$@"`
	else
		TEMP=`getopt -o $short_opts $long_opts -n 'ps_analyze.sh' -- "$@"`
	fi


	# error check #
	if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit -1 ; fi

	# Note the quotes around `$TEMP': they are essential!
	eval set -- "$TEMP"

	while true ; do
		case "$1" in
			--tempdir)
				tempdir_opt="$2"
				if [ -e "$tempdir_opt"  ]; then
					echo "$tempdir_opt already exists, cannot continue."
					exit 1
				fi
            			shift 2
				;;
			-b|--backend)
				if [ "X$2" = "Xzipr" ]; then
					echo "Using Zipr backend."
					export backend="zipr"
					phases_spec=" $phases_spec stratafy_with_pc_confine=off generate_spri=off spasm=off fast_annot=off zipr=on preLoaded_ILR1=off  preLoaded_ILR2=off fast_spri=off create_binary_script=off is_so=off"
					phases_spec=${phases_spec/preLoaded_ILR1=on/}
					phases_spec=${phases_spec/preLoaded_ILR2=on/}
					step_options_gather_libraries="$step_options_gather_libraries --main_exe_only"
				elif [ "X$2" = "Xstrata" ]; then
					echo "Using Strata backend."
					export backend="strata"
					#  strata is default, do nothing.
				fi
            			shift 2
			;;
			-o|--step-option)
           			set_step_option "$2"
            			shift 2
            		;;
            		# This is the watchdog value
#        		-w|--watchdog)
#            			watchdog_val=$2
#            			shift 2
#            		;;
			-s|--step) 
				check_step_option $2
				phases_spec=" $phases_spec $2 "
				shift 2 
			;;
			--manual_test_script) 
				manual_test_script=$2
				shift 2 
			;;
			--manual_test_coverage_file) 
				manual_test_coverage_file=$2
				shift 2 
			;;
			-t|--timeout) 
				set_timer $2 & TIMER_PID=$!
				shift 2 
			;;
			--id) 
				JOBID=$2
				shift 2 
			;;
			--name) 
				DB_PROGRAM_NAME=$2
				shift 2 
			;;
			-h|--help|--usage)
				usage
				exit 1
			;;
			--stop-before)
				stop_before_step=$2
				shift 2
			;;
			--stop-after)
				stop_after_step=$2
				shift 2
			;;
			--dump-before)
				dump_before_step=$2
				shift 2
			;;
			--dump-after)
				dump_after_step=$2
				shift 2
			;;
			--) 	shift 
				break 
			;;
			*) 	echo "Internal error!" 
		 		exit -2 
			;;
		esac
	done

	# report errors if found
	if [ ! -z $1 ]; then
		echo Unparsed parameters:
	fi
	for arg do echo '--> '"\`$arg'" ; done
	if [ ! -z $1 ]; then
		exit -3;	
	fi

	# turn off heaprand, signconv_func_monitor, and watchdog double_free if twitcher is on for now
	is_step_on twitchertransform
	if [[ $? = 1 && "$TWITCHER_HOME" != "" ]]; then
		phases_spec="$phases_spec heaprand=off signconv_func_monitor=off watchdog=off double_free=off"
	fi

	#
	# turn on/off recording of statistics
	#
	is_step_on stats
	if [[ $? = 1 ]]; then
		record_stats=1
	fi
}


#
# subroutine to determine if a particular phase of ps_analyze is on.
#
is_step_on()
{
	local step=$1

	# check for phases explicitly turned off
	echo "$phases_spec"|egrep " $step=off" > /dev/null
	grep_res=$?
	if [ $grep_res -eq 0 ] ; then
		return 0
	fi

	# determine whether phase is on
	echo "$phases_spec"|egrep " $step=on" > /dev/null
	grep_res=$?
	if [ $grep_res -eq 0 ] ; then
		return 1
	fi

	# all steps are off unless explicitly set to on
	return 0
}

#
# is_step_error decide based on the step (in $1) and the exit code (in $2) if there was a failure.
#
is_step_error()
{
	my_step=$1
	my_error=$2


	case $my_step in
		*)
			if [ $my_error -eq 0 ]; then
				# if not otherwise specified, programs should return 0
				return 0;
			fi
			return 1;
	esac
}

#
# return the severity of the error for the step in $1
#
stop_if_error()
{
	my_step=$1

	case $my_step in
		# getting the annotation file right is necessary-ish
		meds_static)
			return 1;
		;;
		# DB operations are necessary 
		pdb_register|clone|fix_calls|fill_in_cfg|fill_in_indtargs|spasm|fast_spri|generate_spri|spasm|stratafy_with_pc_confine)
			return 2;
		;;
		gather_libraries)
			return 3;
		;;
		# other steps are optional
		*)
			return 0;
	esac
}

#
# Check dependencies
#
check_dependencies()
{
	# format is:  step1,step2,step3
	local dependency_list=$1

	# extract each step, make sure step is turned on
	local steps=$(echo $dependency_list | tr "," "\n")
	for s in $steps
	do
		if [[ "$s" != "none" && "$s" != "mandatory" ]]; then
			is_step_on $s
			if [ $? -eq 0 ]; then
				return 0
			fi
		fi
	done

	return 1
}

check_steps_completed()
{
	#echo "Checking steps: $phases_spec"
	for step_spec in $phases_spec
	do
		# if step is on.
		sn=$step_sepc
		sn=$(basename $sn =on)
		sn=$(basename $sn =off)
		is_step_on $sn
		if [ $? = 1 ]; then
			if [ ! -f logs/$sn.log ] ; then
				echo "*********************************************************"
				echo "*********************************************************"
				echo "  Warning! Step requested, but not performed: $step_name "
				echo "*********************************************************"
				echo "*********************************************************"
				warnings=1
			fi
			
		fi
		
	done
}

#
# Detect if this step of the computation is on, and execute it.
#
perform_step()
{
	step=$1
	shift
	mandatory=$1
	shift
	command="$*"

	performed_steps="$performed_steps $step"

	logfile=logs/$step.log

	if [ "$step" = "$stop_before_step" ]; then 
		echo "ps_analyze has been asked to stop before step $step."
		echo "command is:  $command"
		exit 1
	fi
	if [ "$step" = "$dump_before_step" ]; then 
		echo " ---- ps_analyze has been asked to dump before step $step."
		$SECURITY_TRANSFORMS_HOME/plugins_install/dump_map.exe $cloneid > logs/dump_before.log
	fi

	is_step_on $step
	if [ $? -eq 0 ]; then 
		#echo Skipping step $step. [dependencies=$mandatory]
		return 0
	fi

	starttime=`$PS_DATE`

	# optionally record stats
	if [ $record_stats -eq 1 ]; then
		$PEASOUP_HOME/tools/db/job_status_report.sh "$JOBID" "$step" "$stepnum" started "$starttime" inprogress
	fi

	if [[ "$mandatory" != "none" && "$mandatory" != "mandatory" ]]; then
		check_dependencies $mandatory
		if [ $? -eq 0 ]; then 
			echo Skipping step $step because of failed dependencies. [dependencies=$mandatory] "*************************************************"
			errors=1
			if [ $record_stats -eq 1 ]; then
				$PEASOUP_HOME/tools/db/job_status_report.sh "$JOBID" "$step" "$stepnum" completed "$starttime" error
			fi
			return 0
		fi
	fi

	echo -n Performing step "$step" [dependencies=$mandatory] ...
	starttime=`$PS_DATE`

	# If verbose is on, tee to a file 
	if [ ! -z "$DEBUG_STEPS" ]; then
		$command 
		command_exit=$?
	elif [ ! -z "$VERBOSE" ]; then
		$command 2>&1 | tee $logfile
		command_exit=${PIPESTATUS[0]} # this funkiness gets the exit code of $command, not tee
	else
		$command > $logfile 2>&1 
		command_exit=$?
	fi

	endtime=`$PS_DATE`
	
	echo "# ATTRIBUTE start_time=$starttime" >> $logfile
	echo "# ATTRIBUTE end_time=$endtime" >> $logfile
	echo "# ATTRIBUTE peasoup_step_name=$step" >> $logfile
	echo "# ATTRIBUTE peasoup_step_number=$stepnum" >> $logfile
	echo "# ATTRIBUTE peasoup_step_command=$command " >> $logfile
	echo "# ATTRIBUTE peasoup_step_exitcode=$command_exit" >> $logfile

	# report job status
	if [ $command_exit -eq 0 ]; then
		if [ $record_stats -eq 1 ]; then
			$PEASOUP_HOME/tools/db/job_status_report.sh "$JOBID" "$step" "$stepnum" completed "$endtime" success $logfile
		fi
	else
		if [ $record_stats -eq 1 ]; then
			$PEASOUP_HOME/tools/db/job_status_report.sh "$JOBID" "$step" "$stepnum" completed "$endtime" error $logfile
		fi
	fi

	is_step_error $step $command_exit
	if [ $? -ne 0 ]; then
		echo "Done.  Command failed! ***************************************"

		# check if we need to exit
		stop_if_error $step
		if [ $? -gt $error_threshold ]; then 
			echo The $step step is necessary, but failed.  Exiting ps_analyze early.
			exit -1;
		fi
		errors=1
	elif [ -f warning.txt ]; then
		# report warning to user.
		warnings=1
		echo "Done.  Command had serious warnings! ***************************************"
		cat warning.txt
		# report warning in log file, line by line, as an attribute.
		while IFS= read -r line; do
			echo
			echo "# ATTRIBUTE peasoup_serious_warning_text=\"$line\""  >> $logfile
		done < "warning.txt"
		# remove warning.txt so we don't report these warnings again.
		rm -f warning.txt
	else
		echo Done.  Successful.
	fi

	# move to the next step 
	stepnum=`expr $stepnum + 1`

	all_logs="$all_logs $logfile"

	if [ "$step" = "$stop_after_step" ]; then 
		echo "ps_analyze has been asked to stop after step $step."
		echo "command is:  $command"
		exit 1
	fi
	if [ "$step" = "$dump_after_step" ]; then 
		echo " ---- ps_analyze has been asked to dump after step $step."
		$SECURITY_TRANSFORMS_HOME/plugins_install/dump_map.exe $cloneid > logs/dump_after.log
	fi
	return $command_exit
}

do_plugins()
{

	builtin_steps="
		gather_libraries
		meds_static
		pdb_register
		fill_in_cfg
		fill_in_indtargs
		clone
		fix_calls
		manual_test
		zipr
		generate_spri
		preLoaded_ILR1
		preLoaded_ILR2
		spasm
		fast_annot
		fast_spri
	"

	for i in $phases_spec
	do
		stepname=$i
		stepname=$(basename $stepname =on)
		stepname=$(basename $stepname =off)
		
		echo $builtin_steps | grep $stepname  > /dev/null 2> /dev/null 
	
		if [ $? = 0 ]; then
			# skip builtin steps so we don't get errors.
			continue;
		fi
		is_step_on $stepname
		if [ $? = 0 ]; then
			# if step isn't on, don't do it.
			continue
		fi

		# get step options
		this_step_options_name=step_options_$stepname
		value="${!this_step_options_name}"

		plugin_path=$SECURITY_TRANSFORMS_HOME/plugins_install/
		
		# invoke .exe or .sh as a plugin step.
		if [ -x $plugin_path/$stepname.exe ]; then
			perform_step $stepname none $plugin_path/$stepname.exe  $cloneid  $value
		elif [ -x $plugin_path/$stepname.sh ]; then
			perform_step $stepname none $plugin_path/$stepname.sh $cloneid  $value
		else
			echo "*********************************************************"
			echo "*********************************************************"
			echo "  Warning! Step requested, but not performed: $stepname "
			echo "*********************************************************"
			echo "*********************************************************"
			warnings=1
		fi
	done
		

# old style -- scan plugins in alphabetical order.
#	# do plugins directory
#	for i in $SECURITY_TRANSFORMS_HOME/plugins_install/*.exe $SECURITY_TRANSFORMS_HOME/plugins_install/*.sh;
#	do
#		stepname=`basename $i .exe`
#		stepname=`basename $stepname .sh`
#		this_step_options_name=step_options_$stepname
#		value="${!this_step_options_name}"
#		perform_step $stepname none $i $cloneid  $value
#	done

}


#
# create a log for ps_analyze
#
report_logs()
{
	logfile=logs/ps_analyze.log

	myhost=$(hostname)
	echo "# ATTRIBUTE start_time=$ps_starttime" >> $logfile
	echo "# ATTRIBUTE end_time=$ps_endtime" >> $logfile
	echo "# ATTRIBUTE hostname=$myhost" >> $logfile
	echo "# ATTRIBUTE peasoup_step_name=all_peasoup" >> $logfile

#	for i in $all_logs
#	do
#		stepname=`basename $i .log`
#		echo >> $logfile
#		echo ------------------------------------------------------- >> $logfile
#		echo ----- From $i ------------------- >> $logfile
#		echo ------------------------------------------------------- >> $logfile
#		cat $i |sed "s/^# ATTRIBUTE */# ATTRIBUTE ps_$i_/" >> $logfile
#		echo ------------------------------------------------------- >> $logfile
#		echo >> $logfile
#	done
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
			fail_gracefully "PEASOUP ERROR:  $1  not found.  Is there an environment var set incorrectly?"
		fi

		shift 
	done

}

check_for_bad_funcs()
{
	my_name=$1
	bad_funcs="" # previously "iconv_open"
#	bad_funcs=""

	for ducs_i in $bad_funcs
	do
		nm $my_name 2>&1 |grep $ducs_i  > /dev/null 2> /dev/null 
	
		if [ $? = 0 ]; then
			echo "Found bad function ($ducs_i) in executable, we should skip this test."
			echo SKIP
			echo Skip
			echo skip
			exit 255
		fi
	done
}

compatcheck()
{
	infile=$1

	if [ ! -f $infile ]; then
		echo File not found: $infile
		usage
		exit 2
	fi

	file $1 |egrep  "ELF.*executable" > /dev/null 2>&1 
	if [ $? = 0 ]; then
		echo Detected ELF file.
		return
	fi
	file $1 |egrep  "ELF.*shared object" > /dev/null 2>&1 
	if [ $? = 0 ]; then
		echo Detected ELF shared object.
		return
	fi
	file $1 |egrep  "CGC.*executable" > /dev/null 2>&1 
	if [ $? = 0 ]; then
		echo Detected CGCEF file.
		return
	fi


	echo ------------------------
	echo "Input file ($infile) failed compatability check.  Cannot protect this file:"
	file $infile
	echo ------------------------
	usage
	exit 2

}


#
# turn on debugging output if it's requested.
#
if [ ! -z "$VERBOSE" ]; then
	set -x
fi


#
# set the threshold value.  if a step errors with a more severe error (1=most severe, >1 lesser severe)
# than the error_threshold, we exit.
#
error_threshold=0

#
# record when we started processing:
#
ps_starttime=$($PS_DATE)


#
# stepnum used for counting how many steps peasoup executes
# 
stepnum=0


#
# Check for proper environment variables and files that are necessary to peasoupify a program.
#
check_environ_vars PEASOUP_HOME SMPSA_HOME SECURITY_TRANSFORMS_HOME IDAROOT



if [ ! -x $SMPSA_HOME/SMP-analyze.sh ] &&  [ ! -x $SMPSA_HOME/SMP-analyze.sh ] ; then
	echo "SMP-analyze script (local or remote) not found"
	exit 1
fi

#
# Check/parse options
#
if [ -z $2 ]; then
  fail_gracefully "Usage: $0 <original_binary> <new_binary> <options>"
fi

#
# record the original program's name
#
orig_exe=$1
newname=a
shift


#
# sanity check incoming arg.
#
if [ ! -f $orig_exe ]; then
	fail_gracefully "ps_analyze cannot find file named $orig_exe."
fi

#
# record the new program's name
#
export protected_exe=$1
shift

#
# finish argument parsing
#
check_options "$@"


#
# check for input file existance and file type
#
compatcheck $orig_exe

#
# new program
#
name=`basename $orig_exe`

#
# create a new working directory.  default to something that allows parallelism unless asked by the user.
#
if [ "X$tempdir_opt" != "X" ]; then
	newdir="$tempdir_opt"
else
	newdir=peasoup_executable_directory.$JOBID
fi
export newdir

# create a working dir for all our files using the pid
mkdir $newdir

# store the original executable as a.ncexe
cp $orig_exe $newdir/$newname.ncexe

file $orig_exe|grep 32-bit >/dev/null 2>&1 
if [ $? = 0 ]; then 
	if [ `uname -p` = 'x86_64' ]; then
		STRATA_HOME=$STRATA_HOME32
		STRATA=$STRATA32
	fi
	arch_bits=32
else
	arch_bits=64
fi


if [ $backend = "strata" ]; then
	check_envron_vars STRATA_HOME 
	check_files $PEASOUP_HOME/tools/getsyms.sh $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh 
elif [ $backend = "zipr" ]; then
	check_environ_vars ZIPR_INSTALL
	check_files $ZIPR_INSTALL/bin/zipr.exe
else
	echo "Unknown backend!"
	exit 1
fi

#
# setup libstrata.so.  We'll setup two versions, one with symbols so we can debug, and a stripped, faster-loading version.
# by default, use the faster version.  copy in the .symbosl version for debugging
#
if [ -f $STRATA_HOME/lib/libstrata.so  -a $backend = "strata" ]; then
	cp $STRATA_HOME/lib/libstrata.so $newdir/libstrata.so.symbols
	cp $STRATA_HOME/lib/libstrata.so $newdir/libstrata.so.nosymbols
	$PS_STRIP $newdir/libstrata.so.nosymbols
	cp $newdir/libstrata.so.nosymbols $newdir/libstrata.so
fi


adjust_lib_path 



# make sure we overwrite out output file one way or another
rm -f $protected_exe

# and switch to that dir
cd $newdir

check_for_bad_funcs $newname.ncexe

# next, create a location for our log files
mkdir logs 	


#
# turn off runtime protections for BED. turn off runtime prrotections for BED. turn off runtime prrotections for BED.
#
STRATA_DOUBLE_FREE=0
STRATA_HEAPRAND=0
STRATA_PC_CONFINE=0
STRATA_PC_CONFINE_XOR=0


#
# copy the .so files for this exe into a working directory.
#
perform_step gather_libraries mandatory $PEASOUP_HOME/tools/do_gatherlibs.sh $step_options_gather_libraries


#
# Running IDA Pro static analysis phase ...
#
perform_step meds_static mandatory $PEASOUP_HOME/tools/do_idapro.sh $name $step_options_meds_static
touch a.ncexe.annot
cp a.ncexe.annot a.ncexe.annot.full
# this check is extraneous now.
#if [ ! -f $newname.ncexe.annot  ] ; then 
#	fail_gracefully "idapro step failed, exiting early.  Is IDAPRO installed? "
#fi


#
# Run concolic engine
#
#perform_step concolic none $PEASOUP_HOME/tools/do_concolic.sh a -z $PEASOUP_UMBRELLA_DIR/grace.conf

##
## Populate IR Database
##

#
# get some simple info for the program
#	
if [ -z $DB_PROGRAM_NAME ]; then
#	DB_PROGRAM_NAME=`basename $orig_exe | sed "s/[^a-zA-Z0-9]/_/g"`
	DB_PROGRAM_NAME=`basename $protected_exe | sed "s/[^a-zA-Z0-9]/_/g"`
fi
MD5HASH=`$PS_MD5SUM $newname.ncexe | cut -f1 -d' '`

INSTALLER=`pwd`

#
# register the program
#
perform_step pdb_register mandatory "$PEASOUP_HOME/tools/db/pdb_register.sh $DB_PROGRAM_NAME `pwd`" registered.id
is_step_on pdb_register
if [ $? = 1 ]; then
	varid=`cat registered.id`
	if [ ! $varid -gt 0 ]; then
		fail_gracefully "Failed to write Variant into database. Exiting early.  Is postgres running?  Can $PGUSER access the db?"
	fi
fi

if [ $record_stats -eq 1 ]; then
	$PEASOUP_HOME/tools/db/job_spec_register.sh "$JOBID" "$DB_PROGRAM_NAME" "$varid" 'submitted' "$ps_starttime"
fi


if [ $record_stats -eq 1 ]; then
	$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'pending' "$ps_starttime"
fi

# build basic IR
perform_step fill_in_cfg mandatory $SECURITY_TRANSFORMS_HOME/bin/fill_in_cfg.exe $varid	
perform_step fill_in_safefr mandatory $SECURITY_TRANSFORMS_HOME/bin/fill_in_safefr.exe $varid 
perform_step fill_in_indtargs mandatory $SECURITY_TRANSFORMS_HOME/bin/fill_in_indtargs.exe $varid $step_options_fill_in_indtargs

# finally create a clone so we can do some transforms 
perform_step clone mandatory $SECURITY_TRANSFORMS_HOME/bin/clone.exe $varid clone.id
is_step_on clone
if [ $? = 1 ]; then
	cloneid=`cat clone.id`
	#	
	# we could skip this check and simplify ps_analyze if we say that cloning is necessary in is_step_error
	#
	if [ -z "$cloneid" -o  ! "$cloneid" -gt 0 ]; then
		fail_gracefully "Failed to create variant.  Is postgres running properly?"
	fi
fi

# do the basic tranforms we're performing for peasoup 
perform_step fix_calls mandatory $SECURITY_TRANSFORMS_HOME/bin/fix_calls.exe $cloneid	$step_options_fix_calls
# look for strings in the binary 
perform_step find_strings none $SECURITY_TRANSFORMS_HOME/bin/find_strings.exe $cloneid $step_options_find_strings

#
# analyze binary for string signatures
#
perform_step appfw find_strings $PEASOUP_HOME/tools/do_appfw.sh $arch_bits $newname.ncexe logs/find_strings.log $step_optoins_appfw

#
# protect_pov
#
perform_step protect_pov fill_in_indtargs $PEASOUP_HOME/tools/do_protect_pov.sh $PWD/a.ncexe $name $PWD/crash.pov.cso $step_options_protect_pov
if [ -f crash.pov.cso  ]; then
	step_options_watch_allocate="$step_options_watch_allocate --warning_file=crash.pov.cso"
fi

#
# check signatures to determine if we know which program this is.
#
perform_step determine_program find_strings $PEASOUP_HOME/tools/match_program.sh 

# If we ran determine program and got a log, then see if we were successful.
if [ -f logs/determine_program.log ]; then
	program=$(cat logs/determine_program.log |grep "Program is a version of "|sed -e "s/Program is a version of .//" -e "s/.$//")
fi

if [[ "$program" != "" ]]; then
	echo "Detected program is a version of '$program'"

	manual_test_script=$PEASOUP_HOME/tests/$program/test_script.sh

	if [[ -f "$manual_test_script" ]];then
		#check if the selected script succeeds
		#I'm currently capping the validation run to 6 minutes
		#to avoid the case where every test times out, but doesn't
		#invalidate the test. 
		eval timeout 360 $manual_test_script `pwd`/$newname.ncexe `pwd`/$newname.ncexe &>logs/script_validation.log
		
		if [[ ! $? -eq 0 ]]; then
			echo "Manual Script Failure: test script fails to validate original program, ignoring selected script."
			manual_test_script=""
		fi
	else
		echo "Manual Test Script: $manual_test_script Not Found."
		manual_test_script=""
	fi
else
	echo "Program not detected in signature database."
fi

#At this point we will know if manual testing should be turned off automatically
#i.e., we will know if a manual_test_script file exists.
if [ -z $manual_test_script ]; then
	phases_spec=" $phases_spec manual_test=off"
else
	phases_spec=" $phases_spec manual_test=on"
fi

#
# Run script to setup manual tests
#
perform_step manual_test none $PEASOUP_HOME/tools/do_manualtests.sh $name $protected_exe $manual_test_script $manual_test_coverage_file

#
# remove the parts of the annotation file not needed at runtime
#
perform_step fast_annot meds_static $PEASOUP_HOME/tools/fast_annot.sh


#
# sfuzz: simple fuzzing to find crashes and record crashing instruction
# @todo: 2nd arg is the benchmark name but we're currently passing in
#        the binary in
# 
perform_step sfuzz none $PEASOUP_HOME/tools/do_sfuzz.sh $newname.ncexe $orig_exe crash.sfuzz.cso
# if crash found, feed the cso file to the watch allocate step
if [ -f crash.sfuzz.cso  ]; then
	step_options_watch_allocate="$step_options_watch_allocate --warning_file=crash.sfuzz.cso"
fi

#
# cinderella: infer malloc and other libc functions
#
perform_step cinderella clone,fill_in_indtargs,fill_in_cfg $PEASOUP_HOME/tools/do_cinderella.sh $cloneid

#
# For CGC, pad malloc
#
perform_step cgc_hlx cinderella $SECURITY_TRANSFORMS_HOME/bin/cgc_hlx.exe --varid=$cloneid $step_options_cgc_hlx

#
# Do P1/Pn transform.
#
#perform_step p1transform meds_static,clone $PEASOUP_HOME/tools/do_p1transform.sh $cloneid $newname.ncexe $newname.ncexe.annot $PEASOUP_HOME/tools/bed.sh $PN_TIMEOUT_VALUE $step_options_p1transform
		
#
# Do integer transform.
#
if [ -z "$program" ]; then
   program="unknown"
fi

perform_step integertransform meds_static,clone $PEASOUP_HOME/tools/do_integertransform.sh $cloneid $program $CONCOLIC_DIR $INTEGER_TRANSFORM_TIMEOUT_VALUE $step_options_integertransform

#
# perform step to instrument pgm with return shadow stack
#
perform_step ret_shadow_stack meds_static,clone $PEASOUP_HOME/tools/do_rss.sh --varid $cloneid  $step_options_ret_shadow_stack

#
# Do Twitcher transform step if twitcher is present
#
if [[ "$TWITCHER_HOME" != "" && -d "$TWITCHER_HOME" ]]; then
	perform_step twitchertransform none $TWITCHER_HOME/twitcher-transform/do_twitchertransform.sh $cloneid $program $CONCOLIC_DIR $TWITCHER_TRANSFORM_TIMEOUT_VALUE
fi

# input filtering
perform_step input_filtering clone,fill_in_indtargs,fill_in_cfg $SECURITY_TRANSFORMS_HOME/bin/watch_syscall.exe  --varid $cloneid --do_input_filtering $step_options_input_filtering

# watch syscalls
perform_step watch_allocate clone,fill_in_indtargs,fill_in_cfg,pdb_register $SECURITY_TRANSFORMS_HOME/bin/watch_syscall.exe  --varid $cloneid --do_sandboxing $step_options_watch_allocate

#
# check for any steps turned on by the --step option that aren't explicitly mentioned.
# if found, run the step as a plugin to $PS
#
do_plugins

# generate aspri, and assemble it to bspri
perform_step generate_spri mandatory $SECURITY_TRANSFORMS_HOME/bin/generate_spri.exe $($PEASOUP_HOME/tools/is_so.sh a.ncexe) $cloneid a.irdb.aspri

# hack to work with cgc file size restrictions.
stratafier_file=`ls -1 *nostrip 2>/dev/null |head -1` 
if [ "X$stratafier_file" = "X" ]; then 
	stratafier_file=stratafier.o.exe
fi
perform_step spasm mandatory $SECURITY_TRANSFORMS_HOME/bin/spasm a.irdb.aspri a.irdb.bspri a.ncexe $stratafier_file libstrata.so.symbols 

perform_step fast_spri spasm $PEASOUP_HOME/tools/fast_spri.sh a.irdb.bspri a.irdb.fbspri 

# preLoaded_ILR step
perform_step preLoaded_ILR1 fast_spri $STRATA_HOME/tools/preLoaded_ILR/generate_hashfiles.exe a.irdb.fbspri 
perform_step preLoaded_ILR2 preLoaded_ILR1 $PEASOUP_HOME/tools/generate_relocfile.sh a.irdb.fbspri


# put a front end in front of a.stratafied which opens file 990 for strata to read.
perform_step spawner stratafy_with_pc_confine  $PEASOUP_HOME/tools/do_spawner.sh 

# put a front end in front of a.stratafied which opens file 990 for strata to read.
perform_step get_pins spasm,fast_spri  $PEASOUP_HOME/tools/get_pins.sh 

# zipr
perform_step zipr clone,fill_in_indtargs,fill_in_cfg,pdb_register $ZIPR_INSTALL/bin/zipr.exe --variant $cloneid --zipr:objcopy $PS_OBJCOPY $step_options_zipr

# copy TOCTOU tool here if it exists
if [[ "$CONCURRENCY_HOME/toctou_tool" != "" && -d "$CONCURRENCY_HOME/toctou_tool" ]]; then
	perform_step toctou none $CONCURRENCY_HOME/do_toctou.sh
fi

if [[ "$CONCURRENCY_HOME/deadlock" != "" && -d "$CONCURRENCY_HOME/deadlock" ]]; then
    # copy deadlock tool here if it exists
	perform_step deadlock none $CONCURRENCY_HOME/do_deadlock.sh
    # enable some jitter in the scheduling
	perform_step schedperturb none $CONCURRENCY_HOME/do_schedperturb.sh
fi

#
#select the output file name to use -- b.out.addseg if zipr is on.
#
is_step_on zipr
zipr_on=$?
if [ $zipr_on -eq 0 ]; then 
	my_outfile=$newdir/a.sh
else
	my_outfile=$newdir/c.out
fi

# AT 
perform_step cgc_at_string none $DAFFY_HOME/anti_tamper/string_table_trick.sh $(basename $my_outfile)

# Basic sanity check to make sure protected CB is ok
perform_step cgc_sanity_check none $PEASOUP_HOME/tools/cgc_sanity_check.sh $PWD/a.ncexe ${PWD}/$(basename $my_outfile)

#
# create a report for all of ps_analyze.
#
ps_endtime=`$PS_DATE` 
report_logs


# go back to original directory
cd - > /dev/null 2>&1



# copy output file into requested location.
cp $my_outfile $protected_exe

cd $newdir

# gather stats into JSON format
python $PEASOUP_HOME/tools/gather_stats.py logs/*.log > logs/stats.json

# make sure we only do this once there are no more updates to the peasoup_dir
perform_step installer none $PEASOUP_HOME/tools/do_installer.sh $PWD $protected_exe 

cd - > /dev/null 2>&1


# we're done; cancel timer
if [ ! -z $TIMER_PID ]; then
	kill -9 $TIMER_PID
fi

check_steps_completed

#
# return success if we created a script to invoke the pgm and zipr is off. 
#
if [ -f $protected_exe ]; then 
	if [ $errors = 1 ]; then
		echo
		echo
		echo "*******************************"
		echo "* Warning: Some steps failed! *"
		echo "*******************************"
		if [ $record_stats -eq 1 ]; then
			$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'partial' "$ps_endtime" 
		fi
		exit 2;
	elif [ $warnings = 1 ]; then
		echo
		echo
		echo "**********************************************"
		echo "* Warning: Some steps had critical warnings! *"
		echo "**********************************************"
		if [ $record_stats -eq 1 ]; then
			$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'partial' "$ps_endtime" 
		fi
		exit 1;
	
	else
		if [ $record_stats -eq 1 ]; then
			$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'success' "$ps_endtime" 
		fi
		exit 0;
	fi

else
		echo "**************************************"
		echo "*Error: failed to create output file!*"
		echo "*    Cannot protect this program.    *"
		echo "**************************************"
	if [ $record_stats -eq 1 ]; then
		$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'error' "$ps_endtime"
	fi
	exit 255;
fi
