#!/bin/bash
######################################################################
######################################################################
# This file is used as a template, not actually for running the code #
######################################################################
######################################################################

#
# determine the directory that contains the files for peasoup
#
datapath=$1

#
# save original $0
#

origbinpath=$2

#
# make sure we have enough stack space
#
ulimit -s unlimited &>/dev/null

#
# grab the rest of the args in $*
#
shift 2;

#
# Run the program with the proper env. vars set., and the arguments to the program specified
#

command=""
APP_LD_PRELOAD="$LD_PRELOAD"

DO_APPFW=0
if [ "$DO_APPFW" = "1" ]; then 
	command="$command 
		APPFW_LOG_FILE=$datapath/appfw.log
		APPFW_DB=$datapath/appfw.db
		APPFW_SIGNATURE_FILE=$datapath/a.ncexe.sigs.$$
	"
	APP_LD_PRELOAD="$datapath/libappfw.so:$APP_LD_PRELOAD"
fi

DO_TWITCHER=0
if [ "$DO_TWITCHER" = "1" ]; then
	if [ -z $TWITCHER_LOG ]; then
		TWITCHER_LOG=$datapath/twitcher.log
	fi
	command="$command TWITCHER_LOG=$TWITCHER_LOG
	"
	APP_LD_PRELOAD=$datapath/libtwitcher.so:$APP_LD_PRELOAD
fi

DO_TOCTOU=0
if [ "$DO_TOCTOU" = "1" ]; then
	APP_LD_PRELOAD="$datapath/libtoctou_tool.so:$APP_LD_PRELOAD"
fi

DO_DEADLOCK=0
if [ "$DO_DEADLOCK" = "1" ]; then
	if [ -z $DEADLOCK_LOG ]; then
		DEADLOCK_LOG=$datapath/deadlock.log
	fi
	command="$command DEADLOCK_LOG=$DEADLOCK_LOG
	"
	APP_LD_PRELOAD="$datapath/libdeadlock_tool.so:$APP_LD_PRELOAD"
fi


# these are now defaulted nicely by strata for x86-32 and x86-64.
#STRATA_IBTC=1					 
#STRATA_IBTC_SHARED=1
#STRATA_SIEVE=0					
#STRATA_RC=0					
#STRATA_PARTIAL_INLINING=1			

if test `uname -s` = SunOS; then
	command="$command LD_PRELOAD=$datapath/libstrata.so:$APP_LD_PRELOAD"
else
	command="$command LD_PRELOAD=\"$APP_LD_PRELOAD\""
fi


command="$command
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$datapath
PEASOUP_SCHEDULE_PERTURB=0
STRATA_WATCHDOG=0
STRATA_NUM_HANDLE=0
STRATA_DOUBLE_FREE=0
STRATA_HEAPRAND=0
STRATA_SHADOW_STACK=0
STRATA_CONTROLLED_EXIT=0
STRATA_DETECT_SERVERS=0
STRATA_PC_CONFINE=0
STRATA_PC_CONFINE_XOR=0				
STRATA_REKEY_AFTER=0
STRATA_PC_CONFINE_XOR_KEY_LENGTH=1024		
STRATA_ANNOT_FILE=$datapath/a.ncexe.annot 
STRATA_IS_SO=0
STRATA_EXE_FILE=$datapath/a.ncexe
SPAWNER_EXE_FILE=$datapath/spawned
STRATA_MAX_WARNINGS=500000
	exec -a $origbinpath $datapath/a.ncexe \"\$@\""

if [ "$DO_APPFW" = "1" ]; then
#
# setup signatures for appfw
#
BACKTICK=0

addsigs () {
	local sig=$1
	# Make backticks separate strings
	if [[ "$sig" =~ "\`" ]]; then
		if [[ $BACKTICK == 0 ]]; then
			BACKTICK=1
			echo "\`" >> $datapath/a.ncexe.sigs.$$
		fi
		sig=$(echo $sig | tr '\`' ' ')
	fi
	# Split whitespace in arguments and add to sigs
	echo "$sig" | tr ' ' '\n' | /bin/grep -v '^[ \t]*$' >> $datapath/a.ncexe.sigs.$$
}

cp $datapath/a.ncexe.sigs.orig $datapath/a.ncexe.sigs.$$
# only trust command line inputs for files that are not setuid/setgid
if [ ! -g a.ncexe -a ! -u a.ncexe ]; then
	echo $datapath/a.stratafied >> $datapath/a.ncexe.sigs.$$
	echo $origbinpath >> $datapath/a.ncexe.sigs.$$
	echo $PWD >> $datapath/a.ncexe.sigs.$$
	for var in "$@"; do
		addsigs "$var"
		# Add signatures from files with same owner as the executable
		if [[ -f "$var" && $(stat -c %U "$var") == $(stat -c %U $origbinpath) ]]; then
			# limit to first 10K to avoid choking for now
			strings "$var" | head -c 10000 | sort | uniq > $datapath/argfilestrings.$$
			while read line; do
				for s in $line; do
					addsigs "$s"
				done
			done < $datapath/argfilestrings.$$
		fi
	done
fi

unset addsigs
fi

#
#  If STRATA_LOG is clear, no additional logging was requested, and we just always need to log to a file.
#
if [ -z $STRATA_LOG ]; then
	command="STRATA_LOG=detectors STRATA_OUTPUT_FILE=$datapath/diagnostics.out $command"
else
	# otherwise, we add detectors to the strata_log list, and log to stderr	
	command="STRATA_LOG=$STRATA_LOG,detectors $command"
fi

#
# Set mmap files location if preLoaded_ILR is on
#
if [ -f $datapath/a.stratafied.data_hash.ini ]; then
	command="STRATA_SPRI_MMAP_DIR=$datapath/ $command"
fi

#
# Set SPRI file to use (should be generated from the IRDB).
#
# check for faster versions of the spri file first
if [ -f $datapath/a.irdb.fbspri  -o -f $datapath/a.irdb.fbspri.reloc ]; then
	command="STRATA_SPRI_FILE=$datapath/a.irdb.fbspri $command"
# default to the slow one
elif [ -f $datapath/a.irdb.bspri ]; then
	command="STRATA_SPRI_FILE=$datapath/a.irdb.bspri $command"
fi


if [ ! -z $VERBOSE ]; then
	echo $command
fi

eval $command
