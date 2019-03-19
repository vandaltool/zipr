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

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PEASOUP_HOME/irdb-libs/lib

realpath() 
{
  \cd "$1"
  /bin/pwd
}

init_globals()
{


	##################################################################################

	ulimit -s unlimited > /dev/null 2>&1 || true

	# default watchdog value is 30 seconds
	#watchdog_val=30
	errors=0
	warnings=0

	# record statistics in database?
	record_stats=0

	export backend=strata

	# 
	# set default values for 
	#

	#CONCOLIC_DIR=concolic.files_a.stratafied_0001

	# JOBID


	user_critical_steps=""

	# 
	# By default, big data approach is off
	# To turn on the big data approach: modify check_options()
	#

	# alarm handler
	THIS_PID=$$

	#
	# turn off runtime protections for BED. turn off runtime prrotections for BED. turn off runtime prrotections for BED.
	#
	STRATA_DOUBLE_FREE=0
	STRATA_HEAPRAND=0
	STRATA_PC_CONFINE=0
	STRATA_PC_CONFINE_XOR=0

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
	# set library path for shared library builds
	#
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SECURITY_TRANSFORMS_HOME/lib"


	#
	# Remember the last step or step option we parsed, so we can apply future option parsing
	#
	last_step_parsed=""

}
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
	local step_specifier="$1"
	local specifies_step_on=0;
	local specifies_step_off=0;

	# check if step is specified to be off
	echo $step_specifier|egrep "=off$" > /dev/null
	if [[ $? -eq 0 ]]
	then
		specifies_step_off=1
	fi

	# check if step is specified to be on
	echo $step_specifier|egrep "=on$" > /dev/null
	if [[ $? -eq 0 ]] 
	then
		specifies_step_on=1
	fi

	# if user didn't specify, sanity check further 
	if [[ $specifies_step_on -eq 0 ]] && [[ $specifies_step_off -eq 0 ]]
	then
		echo $step_specifier|egrep "=" > /dev/null
		if [[ $? -eq 0 ]]; then
			echo "Malformed option (cannot contain = sign): $1"
			exit -4;
		fi

		# no other odd = things, just go ahead and default to on
		step_specifier="${step_specifier}=on"
		specifies_step_on=1
	fi

	step_name=${step_specifier%%=*}

	echo "$phases_spec"|egrep " $step_name=off " > /dev/null
	local found_off_res=$?
	echo "$phases_spec"|egrep " $step_name=on " > /dev/null
	local found_on_res=$?
	if [[ $specifies_step_on -eq 1 ]] && [[ $found_off_res -eq 0 ]];  then
		echo "Step $step_name specified as both on and off"
		exit -4
	elif [[ $specifies_step_off -eq 1 ]] && [[ $found_on_res -eq 0 ]];  then
		echo "Step $step_name specified as both on and off"
		exit -4
	elif [[ $specifies_step_on -eq 1 ]] && [[ $found_on_res -eq 0 ]];  then
		echo "Step $step_name specified on multiple times"
		exit -4
	elif [[ $specifies_step_off -eq 1 ]] && [[ $found_off_res -eq 0 ]];  then
		echo "Step $step_name specified off multiple times"
		exit -4
	else
		phases_spec=" $phases_spec $step_specifier "
	fi


	# remember for future option parsing
	last_step_parsed=$step_name
}


set_step_option()
{
	step=`echo "$1" | cut -d: -f1` 
	option=`echo "$1" | cut -s -d: -f2-` 
	no_delim_option=`echo "$1" | cut -d: -f99999-` 

	if [[ ! -z $no_delim_option ]]; then
		echo "Detected elided step option in $1"
		set_step_option "$last_step_parsed:$no_delim_option"
		return $?
	fi

	# echo "Found step-option for '$step':'$option'"
	if [[ -z "$option" ]]; then
		echo "Cannot parse step:option pair out of '$1'"
		exit 2
	fi
	last_step_parsed="$step"

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
	echo "Where options can be any of:"
	echo
	echo "   --step <stepname>[=(on|off)]           Turn the <stepname> step on or off."
	echo "   -s <stepname>[=(on|off)]		Same as --step."
	echo "   --critical-step <stepname>[=(on|off)]  Same as --step, but exits with error code if step fails."
	echo "   -c <stepname>[=(on|off)]    		Same as --critical-step."
	echo "   --step-option [<stepname>:]<option>	Pass additional option to step <stepname>.  If stepname "
	echo "                                          is omitted, the last named step (in a -s, -c or -o command"
	echo "                                          is used."
	echo
	echo "   -o <stepname>:<option>			Same as --step-option."
	echo "   --timeout				Specify a timeout for ps_analyze.sh."
	echo "   --help					Print this page."
	echo "   --usage				Same as --help"
	echo "   --tempdir <dir>			Specify where the temporary analysis files are stored, "
	echo "                                          default is peasoup_executable_directory.<exe>.<pid>"
	echo
	echo "   --stop-after <step>			Stop ps_analyze after completing the specified step."
	echo "   --stop-before <step>			Stop ps_analyze before starting the specified step."
	echo "   --dump-after <step>			Dump IR after completing the specified step."
	echo "   --dump-before <step>			Dump IR before starting the specified step."
	echo
	echo "Notes:"
	echo "      1) Steps are applied in the order specified on the command line."
	echo "      2) Options to steps are applied in the order given."

}



#
# check that the remaining options are validly parsable, and record what they are.
#
check_options()
{

	#
	# turn on initial default set of phases
	#

	local default_annot_generator=meds_static
	local initial_on_phases="stratafy_with_pc_confine create_binary_script is_so gather_libraries pdb_register fill_in_cfg fill_in_indtargs clone fix_calls generate_spri spasm fast_annot fast_spri preLoaded_ILR1 preLoaded_ILR2"
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
	short_opts="s:c:t:w:b:o:h"
	long_opts="--long step-option: 
		   --long step: 
		   --long critical-step: 
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
					phases_spec=" $phases_spec gather_libraries=off clone=off stratafy_with_pc_confine=off generate_spri=off spasm=off fast_annot=off preLoaded_ILR1=off  preLoaded_ILR2=off fast_spri=off create_binary_script=off is_so=off"
					phases_spec=${phases_spec/preLoaded_ILR1=on/}
					phases_spec=${phases_spec/preLoaded_ILR2=on/}
					post_phases_spec="$post_phases_spec zipr=on"
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
				shift 2 
			;;
			-c|--critical-step) 
				check_step_option $2
				step_name=$(echo "$2" | sed "s/=on *$//"|sed "s/=off *$//")
				user_critical_steps="$user_critical_steps $step_name "
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

	phases_spec="$phases_spec $post_phases_spec "

	#
	# Check/parse input/output file
	#
	if [ -z $2 ]; then
	  fail_gracefully "Usage: $0 <original_binary> <new_binary> <options>"
	fi

	#
	# record the original program's name
	#
	orig_exe=$1
	shift

	#
	# sanity check incoming arg.
	#
	if [ ! -f $orig_exe ]; then
		fail_gracefully "ps_analyze cannot find file named $orig_exe."
	fi


	is_step_on rida
	local rida_on=$?
	is_step_on meds_static
	local meds_static_on=$?
	# if both are on, that's an error
	if [[ $rida_on -eq 1 ]] && [[ $meds_static_on -eq 1 ]]; then
		echo "Cannot enable both rida and meds_static"
		exit -4
	# if neither are on, use default
	elif [[ $rida_on -eq 0 ]] && [[ $meds_static_on -eq 0 ]]; then
		phases_spec=" $phases_spec ${default_annot_generator}=on "
	fi
	# else, exactly 1 must be on, and that needs no special handling.

	# double check that we didn't turn both off.
	is_step_on rida
	rida_on=$?
	is_step_on meds_static
	meds_static_on=$?
	if [[ $rida_on -eq 0 ]] && [[ $meds_static_on -eq 0 ]]; then
		echo "Cannot explicitly disable both rida and meds_static (or disable meds_static without enabling rida)"
		exit -4
	fi
		
	


	# record a job id
	JOBID="$(basename $orig_exe).$$"

	#
	# record the new program's name
	#
	export protected_exe=$1
	shift

	# report errors if found
	if [ ! -z $1 ]; then
		echo Unparsed parameters:
	fi
	for arg do echo '--> '"\`$arg'" ; done
	if [ ! -z $1 ]; then
		exit -3;	
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
	my_error=$1
	my_step=$2

	case $my_step in
		*)
			if [[ $my_error -eq 0 ]]; then
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

	# check for a step the user specified as critical.
	echo "$user_critical_steps"|egrep " $my_step " > /dev/null
	grep_res=$?
	if [ $grep_res -eq 0 ] ; then
		return 4;
	fi

	case $my_step in
		# getting the annotation file right is necessary-ish
		meds_static|rida)
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
				echo "  (Could not find ${step_name}.exe nor lib${step_name}.so"
				echo "  in search path: ${PSPATH}                              "
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

	echo "$command"|grep "thanos.exe " > /dev/null
        grep_res=$?
        using_thanos=!$grep_res

	if [[ $using_thanos -eq 0 ]]; then
		logfile=logs/$step.log
	else
		logfile=logs/thanos.log
	fi

	if [ "$step" = "$stop_before_step" ]; then 
		echo "ps_analyze has been asked to stop before step $step."
		echo "command is:  LD_LIBRARY_PATH=$PEASOUP_HOME/irdb-libs/lib gdb --args $command"	
		exit 1
	fi
	if [ "$step" = "$dump_before_step" ]; then 
		echo " ---- ps_analyze has been asked to dump before step $step."	
		$PEASOUP_HOME/irdb-libs/plugins_install/dump_map.exe $cloneid > logs/dump_before.log
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

	starttime=`$PS_DATE`

		
	# If verbose is on, tee to a file 
	if [[ ! -z "$DEBUG_STEPS" ]]; then
		echo -n Performing step "$step" [dependencies=$mandatory] ...
		eval $command 
		command_exit=$?
	elif [[ ! -z "$VERBOSE" && $using_thanos -eq 0 ]]; then
		echo -n Performing step "$step" [dependencies=$mandatory] ...
		eval $command 2>&1 | tee $logfile
		command_exit=${PIPESTATUS[0]} # this funkiness gets the exit code of $command, not tee
	elif [[ ! -z "$VERBOSE" && $using_thanos -ne 0 ]]; then
		echo -n Performing step "$step" [dependencies=$mandatory] ...
		eval $command > $logfile 2>&1
		command_exit=$?
		# display logs to stdout
		for this_step in $step
		do
			cat logs/$this_step.log
		done
		cat $logfile
	elif [[ $using_thanos -ne 0 ]]; then
		eval $command
		command_exit=$?
	else
		echo -n Performing step "$step" [dependencies=$mandatory] ...
		eval $command > $logfile 2>&1 
		command_exit=$?
	fi
	
	endtime=`$PS_DATE`
	
	echo "#ATTRIBUTE start_time=$starttime" >> $logfile
	echo "#ATTRIBUTE end_time=$endtime" >> $logfile
	echo "#ATTRIBUTE step_name=$step" >> $logfile
	echo "#ATTRIBUTE step_number=$stepnum" >> $logfile
	echo "#ATTRIBUTE step_command=$command " >> $logfile
	echo "#ATTRIBUTE step_exitcode=$command_exit" >> $logfile

	# report job status
	if [[ $command_exit -eq 0 ]]; then
		if [[ $record_stats -eq 1 ]]; then
			$PEASOUP_HOME/tools/db/job_status_report.sh "$JOBID" "$step" "$stepnum" completed "$endtime" success $logfile
		fi
	else
		if [[ $record_stats -eq 1 ]]; then
			$PEASOUP_HOME/tools/db/job_status_report.sh "$JOBID" "$step" "$stepnum" completed "$endtime" error $logfile
		fi
	fi

	is_step_error $command_exit $step
	if [[ $? -ne 0 ]]; then
		if [[ $using_thanos -eq 0 || $command_exit -ne 1 ]]; then
			echo "Done.  Command failed! ***************************************"
		fi

		# check if we need to exit
		stop_if_error $step
		if [[ $using_thanos -ne 0 ]]; then
			if [[ $command_exit -ne 1 ]]; then
	                       	echo "A critical step executed under the thanos plugin driver has been forcefully terminated. Exiting ps_analyze early."
			fi
                        exit -1;
		elif [ $? -gt $error_threshold ]; then 
			echo "The $step step is necessary, but failed.  Exiting ps_analyze early."
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
			echo "#ATTRIBUTE serious_warning_text=\"$line\""  >> $logfile
		done < "warning.txt"
		# remove warning.txt so we don't report these warnings again.
		rm -f warning.txt
	else
		if [[ $using_thanos -eq 0 ]]; then
			echo "Done.  Successful."
		fi
	fi

	# move to the next step 
	stepnum=`expr $stepnum + 1`

	if [[ $using_thanos -ne 0 ]]; then
		for this_step in $step
        	do
        		all_logs="$all_logs logs/$this_step.log"
        	done
	fi
	all_logs="$all_logs $logfile"
	
	if [ "$step" = "$stop_after_step" ]; then 
		echo "ps_analyze has been asked to stop after step $step."
		echo "command is:  LD_LIBRARY_PATH=$SECURITY_TRANSFORMS_HOME/lib gdb --args $command"
		exit 1
	fi
	if [ "$step" = "$dump_after_step" ]; then 
		echo " ---- ps_analyze has been asked to dump after step $step."
		$PEASOUP_HOME/irdb-libs/plugins_install/dump_map.exe $cloneid > logs/dump_after.log
	fi
	return $command_exit
}

run_current_thanos_steps()
{
	# echo "Doing thanos steps: $thanos_plugins"
	# execute last block of thanos plugins if there are any left	
	if [[ $thanos_plugins ]]; then
		perform_step "$thanos_steps" none "$PEASOUP_HOME/irdb-libs/plugins_install/thanos.exe "$thanos_plugins""
                thanos_plugins=""
                thanos_steps=""		
	fi
}


find_plugin()
{
	local plugin_name=$1

	for i in ${PSPATH//:/ }
	do
                if [[ -x $i/lib$stepname.so ]]; then
			echo "$i/lib$stepname.so"
			return
		elif [[ -x $i/$stepname.exe ]]; then
			echo "$i/$stepname.exe" 
			return
		elif [[ -x $i/$stepname.sh ]]; then
			echo "$i/$stepname.sh" 
			return
		fi
	done

}


do_plugins()
{

	builtin_steps="
		gather_libraries
		meds_static
		rida
		pdb_register
		clone
		manual_test
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
			continue
		fi
		is_step_on $stepname
		if [ $? = 0 ]; then
			# if step isn't on, don't do it.
			continue
		fi

		# get step options
		this_step_options_name=step_options_$stepname
		value="${!this_step_options_name}"

		plugin_path=$(find_plugin $stepname)

		# first check if step can be invoked as a thanos plugin
		if [[ "$plugin_path" == *.so ]]; then

			# if this step is a stop before/after step, cleanup anything outstanding so we can do the one step special.
			if [[ $stepname == $stop_before_step ]] || [[ $stepname == $stop_after_step ]] ||
			   [[ $stepname == $dump_before_step ]] || [[ $stepname == $dump_after_step ]]; then
				run_current_thanos_steps
			fi

			# add step to the block of contiguous thanos plugins
			stop_if_error $stepname			
			if [[ $? -gt $error_threshold ]]; then
                        	thanos_plugins="$thanos_plugins \"$plugin_path --step-args $cloneid $value\""
			else
				thanos_plugins="$thanos_plugins \"$plugin_path -optional --step-args $cloneid $value\""	
			fi
			thanos_steps="$thanos_steps $stepname"

			# if this step is a stop before/after step, do it special, so we exit early.
			if [[ $stepname == $stop_before_step ]] || [[ $stepname == $stop_after_step ]]; then
				# just run the step now.
				perform_step $stepname none "$plugin_path/thanos.exe --no-redirect "$thanos_plugins""
				thanos_steps=""
				thanos_plugins=""
			elif   [[ $stepname == $dump_before_step ]] || [[ $stepname == $dump_after_step ]]; then
				# just run the step now.
				perform_step $stepname none "$plugin_path/thanos.exe "$thanos_plugins""
				thanos_steps=""
				thanos_plugins=""
			fi
			continue
		elif [[ $thanos_steps ]]; then 
			# execute preceding block of thanos plugin steps now
			run_current_thanos_steps
		fi
		
		# invoke .exe, or .sh as a plugin step
		if [[ "$plugin_path" == *.exe ]]; then
			perform_step $stepname none $plugin_path $cloneid  $value
		elif [[ "$plugin_path" == *.sh ]]; then
			perform_step $stepname none $plugin_path $cloneid  $value
		else
			echo "*********************************************************"
			echo "*********************************************************"
			echo "  Warning! Step requested, but not performed: $stepname  "
			echo "  (Could not find ${stepname}.exe nor lib${stepname}.so  "
			echo "  in search path: ${PSPATH})                             "
			echo "*********************************************************"
			echo "*********************************************************"
			warnings=1
		fi
	done

	# execute last block of thanos plugins if there are any left	
	run_current_thanos_steps

}


#
# create a log for ps_analyze
#
report_logs()
{
	logfile=logs/ps_analyze.log

	myhost=$(hostname)
	echo "#ATTRIBUTE start_time=$ps_starttime" >> $logfile
	echo "#ATTRIBUTE end_time=$ps_endtime" >> $logfile
	echo "#ATTRIBUTE hostname=$myhost" >> $logfile
	echo "#ATTRIBUTE step_name=all_helix" >> $logfile

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


do_prefix_steps()
{
	#
	# copy the .so files for this exe into a working directory.
	#
	perform_step gather_libraries mandatory $PEASOUP_HOME/tools/do_gatherlibs.sh $step_options_gather_libraries

	#
	# Running IDA Pro static analysis phase ...
	#
	perform_step meds_static mandatory $PEASOUP_HOME/tools/do_idapro.sh $name $step_options_meds_static
	perform_step rida mandatory $PEASOUP_HOME/irdb-libs/plugins_install/rida.exe ./a.ncexe ./a.ncexe.annot ./a.ncexe.infoannot ./a.ncexe.STARSxrefs $step_options_rida
	touch a.ncexe.annot
	cp a.ncexe.annot a.ncexe.annot.full

	##
	## Populate IR Database
	##

	#
	# get some simple info for the program
	#	
	if [ -z $DB_PROGRAM_NAME ]; then
		DB_PROGRAM_NAME=`basename $protected_exe | sed "s/[^a-zA-Z0-9]/_/g"`
	fi
	#MD5HASH=`$PS_MD5SUM $newname.ncexe | cut -f1 -d' '`

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
}

main() 
{
	init_globals



	#
	# Check for proper environment variables and files that are necessary to peasoupify a program.
	#
	check_environ_vars PEASOUP_HOME 

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
	newname=a

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
		check_environ_vars STRATA_HOME 
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


	do_prefix_steps
	cloneid=$varid


	do_plugins


	#
	# create a report for all of ps_analyze.
	#
	ps_endtime=`$PS_DATE` 
	report_logs

	# figure out the output file
	is_step_on zipr
	zipr_on=$?
	if [ $zipr_on -eq 0 ]; then 
		my_outfile=$newdir/a.sh
	else
		my_outfile=$newdir/c.out
	fi

	# go back to original directory
	cd - > /dev/null 2>&1

	# copy output file into requested location.
	cp $my_outfile $protected_exe

	cd $newdir

	# gather stats into JSON format
	#python $PEASOUP_HOME/tools/gather_stats.py logs/*.log > logs/stats.json

	# make sure we only do this once there are no more updates to the peasoup_dir
	#perform_step installer none $PEASOUP_HOME/tools/do_installer.sh $PWD $protected_exe

	
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
}

main "$@"
