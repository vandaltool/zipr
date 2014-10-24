#!/bin/bash
#
# ps_analyze.sh - analyze a program and transform it for peasoupification to prevent exploit.
#
# This script depends on having many environment variables defined, but it should check that they are defined properly for you.
#
# Usage:
#     peasoup_analyze.sh <original_binary> <new_binary> <options>
#


##################################################################################
# set default values for 
##################################################################################

initial_off_phases="isr ret_shadow_stack determine_program stats fill_in_safefr zipr"


##################################################################################







ulimit -s unlimited

# default watchdog value is 30 seconds
watchdog_val=30
errors=0

# record statistics in database?
record_stats=0

# DEFAULT TIMEOUT VALUE
INTEGER_TRANSFORM_TIMEOUT_VALUE=1800
TWITCHER_TRANSFORM_TIMEOUT_VALUE=1800
# Setting PN timeout to 6 hours for TNE. 
PN_TIMEOUT_VALUE=21600

#non-zero to use canaries in PN/P1, 0 to turn off canaries
#DO_CANARIES=1
#on for on and off for off
DO_CANARIES=on
CONCOLIC_DIR=concolic.files_a.stratafied_0001

intxform_warnings_only=0  # default: integer warnings only mode is off
intxform_detect_fp=1      # default: detect benign false positives is on
                          #   but if determine_program is off, it's a no-op
intxform_instrument_idioms=0  # default: do not instrument instructions marked as IDIOM by STARS

# JOBID

JOBID="$(basename $1)-$$"

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
	LD_LIBRARY_PATH=$NEWPATH:$PWD/$newdir
}

check_step_option()
{
	echo $1|egrep "=off$|=on$" > /dev/null
	if [ $? -ne 0 ]; then
		echo Malformed option: $1;
		exit -4;
	fi
	
}

set_p1transform_option()
{
	option=`echo $1 | sed 's/\(.*\)=.*/\1/'`
	val=`echo $1 | sed 's/.*=\(.*\)/\1/'`

	case "$option" in
		canaries)
			DO_CANARIES=$val
			;;
		*) echo "Unrecognized p1transform option: $option\n"
			exit -2 #What's the correct exit status?
			;;
	esac

}

set_step_option()
{
	step=`echo $1 | sed 's/\(.*\):.*/\1/'`
	option=`echo $1 | sed 's/.*:\(.*\)/\1/'`

	case "$step" in
       p1transform)
           	set_p1transform_option $option
            ;;
		*) 	echo "Unrecognized --step-option: $step" 
		 	exit -2 #What's the correct exit status?
			;;
	esac
	
}



#
# check that the remaining options are validly parsable, and record what they are.
#
check_options()
{

	# 
	# loop to process options.
	# 

	# Note that we use `"$@"' to let each command-line parameter expand to a 
	# separate word. The quotes around `$@' are essential!
	# We need TEMP as the `eval set --' would nuke the return value of getopt.
	TEMP=`getopt -o s:t:w: --long step-option: --long integer_warnings_only --long integer_instrument_idioms --long integer_detect_fp --long no_integer_detect_fp --long step: --long timeout: --long id: --long manual_test_script: --long manual_test_coverage_file: --long watchdog: -n 'ps_analyze.sh' -- "$@"`

	# error check #
	if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit -1 ; fi

	# Note the quotes around `$TEMP': they are essential!
	eval set -- "$TEMP"

	while true ; do
		case "$1" in
			--step-option)
           	set_step_option $2
            shift 2
            ;;
        -w|--watchdog)
            # This is the watchdog value
            watchdog_val=$2
            shift 2
            ;;
		-s|--step) 
			check_step_option $2
			phases_off=" $phases_off $2 "
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
		--integer_warnings_only)
			echo "integer transform: warnings only enabled"
			intxform_warnings_only=1
			shift 
			;;
		--no_integer_detect_fp)
			echo "integer transform: benign false positive detection disabled"
			intxform_detect_fp=0
			shift 
			;;
		--integer_detect_fp)
			echo "integer transform: benign false positive detection enabled"
			intxform_detect_fp=1
			shift 
			;;
		--integer_instrument_idioms)
			echo "integer transform: instrument idioms"
			intxform_instrument_idioms=1
			shift 
			;;
		-t|--timeout) 
			set_timer $2 & TIMER_PID=$!
			shift 2 
			;;
		--id) 
			JOBID=$2
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

#	if [ -z $manual_test_script ]; then
#		phases_off=" $phases_off manual_test=off"
#	else
#		phases_off=" $phases_off manual_test=on"
#	fi

	# report errors if found
	if [ ! -z $1 ]; then
		echo Unparsed parameters:
	fi
	for arg do echo '--> '"\`$arg'" ; done
	if [ ! -z $1 ]; then
		exit -3;	
	fi

	for phase in $initial_off_phases
	do

		# --step $phase=(on|off) not specified on the command line
		# default policy is off
		# to make the default policy on, get rid of this block of code
		echo $phases_off|egrep "$phase=" > /dev/null
		if [ ! $? -eq 0 ];
		then
			# by default it's off
			phases_off="$phases_off $phase=off"
		fi
	done

	# turn off heaprand and double_free if twitcher is on for now
	is_step_on twitchertransform
	if [[ $? = 1 && "$TWITCHER_HOME" != "" ]]; then
		phases_off="$phases_off heaprand=off double_free=off"
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

	echo $phases_off|egrep "$step=off" > /dev/null
	if [ $? -eq 0 ] ; then
		return 0
	fi

	# for now, all steps are on unless explicitly set to off
	return 1
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
		pdb_register|clone|fix_calls|fill_in_indtargs|spasm|fast_spri|generate_spri|spasm|stratafy_with_pc_confine)
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

	logfile=logs/$step.log

	is_step_on $step
	if [ $? -eq 0 ]; then 
		echo Skipping step $step. [dependencies=$mandatory]
		return 0
	fi

	starttime=`date --iso-8601=seconds`

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

	endtime=`date --iso-8601=seconds`
	
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
	else
		echo Done.  Successful.
	fi

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

	echo "# ATTRIBUTE start_time=$ps_starttime" >> $logfile
	echo "# ATTRIBUTE end_time=$ps_endtime" >> $logfile
	echo "# ATTRIBUTE peasoup_step_name=all_peasoup" >> $logfile

	for i in $all_logs
	do
		stepname=`basename $i .log`
		echo >> $logfile
		echo ------------------------------------------------------- >> $logfile
		echo ----- From $i ------------------- >> $logfile
		echo ------------------------------------------------------- >> $logfile
		cat $i |sed "s/^# ATTRIBUTE */# ATTRIBUTE ps_$i_/" >> $logfile
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
ps_starttime=`date --iso-8601=seconds`


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

#
# setup libstrata.so.  We'll setup two versions, one with symbols so we can debug, and a stripped, faster-loading version.
# by default, use the faster version.  copy in the .symbosl version for debugging
#
cp $STRATA_HOME/lib/libstrata.so $newdir/libstrata.so.symbols
cp $STRATA_HOME/lib/libstrata.so $newdir/libstrata.so.nosymbols
strip $newdir/libstrata.so.nosymbols
cp $newdir/libstrata.so.nosymbols $newdir/libstrata.so


adjust_lib_path 



# make sure we overwrite out output file one way or another
rm -f $stratafied_exe

# and switch to that dir
cd $newdir

check_for_bad_funcs $newname.ncexe

# next, create a location for our log files
mkdir logs 	

#
# create a stratafied binary that does pc confinement.
#
perform_step stratafy_with_pc_confine mandatory sh $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh $newname.ncexe $newname.stratafied 

#
# Let's output the modified binary
# This binary will really be a shell script that calls the newly stratafied binary
#
perform_step create_binary_script 	mandatory $PEASOUP_HOME/tools/do_makepeasoupbinary.sh $name 
perform_step heaprand 	 		pc_confine,double_free $PEASOUP_HOME/tools/update_env_var.sh STRATA_HEAPRAND 1
perform_step controlled_exit none 		 	 $PEASOUP_HOME/tools/update_env_var.sh STRATA_CONTROLLED_EXIT 1
perform_step detect_server  pc_confine  $PEASOUP_HOME/tools/update_env_var.sh STRATA_DETECT_SERVERS 1
#perform_step ibtc  none  $PEASOUP_HOME/tools/update_env_var.sh STRATA_IBTC 0
#perform_step sieve  none  $PEASOUP_HOME/tools/update_env_var.sh STRATA_SIEVE 1
#perform_step return_cache  none  $PEASOUP_HOME/tools/update_env_var.sh STRATA_RC 1
#perform_step partial_inlining  none  $PEASOUP_HOME/tools/update_env_var.sh STRATA_PARTIAL_INLINING 0
perform_step rekey  none  $PEASOUP_HOME/tools/update_env_var.sh STRATA_REKEY_AFTER 5000
perform_step double_free heaprand $PEASOUP_HOME/tools/update_env_var.sh STRATA_DOUBLE_FREE 1
perform_step pc_confine  none $PEASOUP_HOME/tools/update_env_var.sh STRATA_PC_CONFINE 1
perform_step isr 	 pc_confine $PEASOUP_HOME/tools/update_env_var.sh STRATA_PC_CONFINE_XOR 1
perform_step watchdog 	 signconv_func_monitor $PEASOUP_HOME/tools/update_env_var.sh STRATA_WATCHDOG $watchdog_val
perform_step is_so 	 mandatory $PEASOUP_HOME/tools/update_env_var.sh STRATA_IS_SO $($PEASOUP_HOME/tools/is_so.sh a.ncexe)

# turn on sign conversion function monitoring
perform_step signconv_func_monitor heaprand $PEASOUP_HOME/tools/update_env_var.sh STRATA_NUM_HANDLE 1


#
# turn off runtime prrotections for BED. turn off runtime prrotections for BED. turn off runtime prrotections for BED.
#
STRATA_DOUBLE_FREE=0
STRATA_HEAPRAND=0
STRATA_PC_CONFINE=0
STRATA_PC_CONFINE_XOR=0


#
# copy the .so files for this exe into a working directory.
#
perform_step gather_libraries mandatory $PEASOUP_HOME/tools/do_gatherlibs.sh


#
# Running IDA Pro static analysis phase ...
#
perform_step meds_static mandatory $PEASOUP_HOME/tools/do_idapro.sh
touch a.ncexe.annot
# this check is extraneous now.
#if [ ! -f $newname.ncexe.annot  ] ; then 
#	fail_gracefully "idapro step failed, exiting early.  Is IDAPRO installed? "
#fi


#
# Run concolic engine
#
perform_step concolic none $PEASOUP_HOME/tools/do_concolic.sh a -z $PEASOUP_UMBRELLA_DIR/grace.conf

##
## Populate IR Database
##

#
# get some simple info for the program
#	
DB_PROGRAM_NAME=`basename $orig_exe.$$ | sed "s/[^a-zA-Z0-9]/_/g"`
DB_PROGRAM_NAME="psprog_$DB_PROGRAM_NAME"
MD5HASH=`md5sum $newname.ncexe | cut -f1 -d' '`

INSTALLER=`pwd`

#
# register the program
#
perform_step pdb_register mandatory "$PEASOUP_HOME/tools/db/pdb_register.sh $DB_PROGRAM_NAME `pwd`" registered.id
varid=`cat registered.id`
if [ ! $varid -gt 0 ]; then
	fail_gracefully "Failed to write Variant into database. Exiting early.  Is postgres running?  Can $PGUSER access the db?"
fi

if [ $record_stats -eq 1 ]; then
	$PEASOUP_HOME/tools/db/job_spec_register.sh "$JOBID" "$DB_PROGRAM_NAME" "$varid" 'submitted' "$ps_starttime"
fi


if [ $record_stats -eq 1 ]; then
	$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'pending' "$ps_starttime"
fi

# build basic IR
perform_step fill_in_cfg mandatory $SECURITY_TRANSFORMS_HOME/libIRDB/test/fill_in_cfg.exe $varid	
perform_step fill_in_safefr mandatory $SECURITY_TRANSFORMS_HOME/tools/safefr/fill_in_safefr.exe $varid 
perform_step fill_in_indtargs mandatory $SECURITY_TRANSFORMS_HOME/libIRDB/test/fill_in_indtargs.exe $varid 

# finally create a clone so we can do some transforms 
perform_step clone mandatory $SECURITY_TRANSFORMS_HOME/libIRDB/test/clone.exe $varid clone.id
cloneid=`cat clone.id`

#	
# we could skip this check and simplify ps_analyze if we say that cloning is necessary in is_step_error
#
if [ -z "$cloneid" -o  ! "$cloneid" -gt 0 ]; then
	fail_gracefully "Failed to create variant.  Is postgres running properly?"
fi

# do the basic tranforms we're performing for peasoup 
perform_step fix_calls mandatory $SECURITY_TRANSFORMS_HOME/libIRDB/test/fix_calls.exe $cloneid	
#gdb --args $SECURITY_TRANSFORMS_HOME/libIRDB/test/fix_calls.exe $cloneid	



# look for strings in the binary 
perform_step find_strings none $SECURITY_TRANSFORMS_HOME/libIRDB/test/find_strings.exe $cloneid

#
# analyze binary for string signatures
#
perform_step appfw find_strings $PEASOUP_HOME/tools/do_appfw.sh $arch_bits $newname.ncexe logs/find_strings.log

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
	phases_off=" $phases_off manual_test=off"
else
	phases_off=" $phases_off manual_test=on"
fi

#
# Run script to setup manual tests
#
perform_step manual_test none $PEASOUP_HOME/tools/do_manualtests.sh $name $stratafied_exe $manual_test_script $manual_test_coverage_file

#
# remove the parts of the aannotation file not needed at runtime
#
perform_step fast_annot preLoaded_ILR2 $PEASOUP_HOME/tools/fast_annot.sh


#
# Do P1/Pn transform.
#
perform_step p1transform meds_static,clone $PEASOUP_HOME/tools/do_p1transform.sh $cloneid $newname.ncexe $newname.ncexe.annot $PEASOUP_HOME/tools/bed.sh $PN_TIMEOUT_VALUE $DO_CANARIES

		
#
# Do integer transform.
#
if [ -z "$program" ]; then
   program="unknown"
fi
perform_step integertransform meds_static,clone $PEASOUP_HOME/tools/do_integertransform.sh $cloneid $program $CONCOLIC_DIR $INTEGER_TRANSFORM_TIMEOUT_VALUE $intxform_warnings_only $intxform_detect_fp $intxform_instrument_idioms

#
# perform_calc -- get some stats about the DB
#
#perform_step calc_conflicts none $SECURITY_TRANSFORMS_HOME/libIRDB/test/calc_conflicts.exe $cloneid a.ncexe

#
# perform step to instrument pgm with return shadow stack
#
perform_step ret_shadow_stack meds_static,clone $PEASOUP_HOME/tools/do_rss.sh $cloneid 

#
# Do Twitcher transform step if twitcher is present
#
if [[ "$TWITCHER_HOME" != "" && -d "$TWITCHER_HOME" ]]; then
	perform_step twitchertransform none $TWITCHER_HOME/twitcher-transform/do_twitchertransform.sh $cloneid $program $CONCOLIC_DIR $TWITCHER_TRANSFORM_TIMEOUT_VALUE
fi

# only do ILR for main objects that aren't relocatable.  reloc. objects 
# are still buggy for ILR
if [ $($PEASOUP_HOME/tools/is_so.sh a.ncexe) = 0 ]; then
	perform_step ilr none $SECURITY_TRANSFORMS_HOME/libIRDB/test/ilr.exe $cloneid 
fi

# generate aspri, and assemble it to bspri
perform_step generate_spri mandatory $SECURITY_TRANSFORMS_HOME/libIRDB/test/generate_spri.exe $($PEASOUP_HOME/tools/is_so.sh a.ncexe) $cloneid a.irdb.aspri
perform_step spasm mandatory $SECURITY_TRANSFORMS_HOME/tools/spasm/spasm a.irdb.aspri a.irdb.bspri a.ncexe stratafier.o.exe libstrata.so.symbols 
perform_step fast_spri spasm $PEASOUP_HOME/tools/fast_spri.sh a.irdb.bspri a.irdb.fbspri 

# preLoaded_ILR step
perform_step preLoaded_ILR1 fast_spri $STRATA_HOME/tools/preLoaded_ILR/generate_hashfiles.exe a.irdb.fbspri 
perform_step preLoaded_ILR2 preLoaded_ILR1 $PEASOUP_HOME/tools/generate_relocfile.sh a.irdb.fbspri

# zipr
perform_step zipr clone,fill_in_indtargs,fill_in_cfg,meds2pdb $ZIPR_HOME/bin/zipr.exe $cloneid

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
# create a report for all of ps_analyze.
#
ps_endtime=`date --iso-8601=seconds`
report_logs


# go back to original directory
cd - > /dev/null 2>&1

cp $newdir/$name.sh $stratafied_exe

# we're done; cancel timer
if [ ! -z $TIMER_PID ]; then
	kill -9 $TIMER_PID
fi

# return success if we created a script to invoke the pgm. 
if [ -f $stratafied_exe ]; then 
	if [ $errors = 1 ]; then
		echo
		echo
		echo "*****************************"
		echo "*Warning: Some steps failed!*"
		echo "*****************************"
		if [ $record_stats -eq 1 ]; then
			$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'partial' "$ps_endtime" "$INSTALLER"
		fi
	else
		if [ $record_stats -eq 1 ]; then
			$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'success' "$ps_endtime" "$INSTALLER"
		fi
	fi

	exit 0;
else
	if [ $record_stats -eq 1 ]; then
		$PEASOUP_HOME/tools/db/job_spec_update.sh "$JOBID" 'error' "$ps_endtime"
	fi
	exit 255;
fi
