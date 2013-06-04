#!/bin/bash
#
# do_p1transform.sh <originalBinary> <MEDS annotationFile> <cloneId> <BED_script>
#
# pre: we are in the top-level directory created by ps_analyze.sh
#

# input
CLONE_ID=$1
ORIGINAL_BINARY=$2
MEDS_ANNOTATION_FILE=$3
BED_SCRIPT=$4
TIMEOUT_VALUE=$5
DO_CANARIES=$6
# Maximum length of time to spend replaying inputs to get coverage
COVERAGE_REPLAY_TIMEOUT=600
# Timeout per replayer run
REPLAYER_TIMEOUT=120
TOP_LEVEL=`pwd`
BASELINE_DIR=$TOP_LEVEL/replayer_baseline
INPUT_CUTOFF=50

mkdir $BASELINE_DIR

STRATAFIED_BINARY=$TOP_LEVEL/a.stratafied

# configuration variables
P1_DIR=p1.xform
CONCOLIC_DIR=concolic.files_a.stratafied_0001
#EXECUTED_ADDRESSES=$CONCOLIC_DIR/executed_address_list.txt
EXECUTED_ADDRESSES_CONCOLIC=concolic_coverage.txt;
EXECUTED_ADDRESSES_MANUAL=manual_coverage.txt
EXECUTED_ADDRESSES_FINAL=final.coverage.txt
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt
BLACK_LIST=$P1_DIR/p1.filtered_out        # list of functions to blacklist
COVERAGE_FILE=$P1_DIR/p1.coverage
P1THRESHOLD=0.5


PN_BINARY=$SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe

echo "P1: transforming binary: cloneid=$CLONE_ID bed_script=$BED_SCRIPT timeout_value=$TIMEOUT_VALUE"

execute_pn()
{
	echo "P1: issuing command: $SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe $1 $2 $3 $4 $5 $6 with timeout value=$TIMEOUT_VALUE"

	# On timeout send SIGUSR1 (signal #10)
	timeout -10 $TIMEOUT_VALUE $PN_BINARY $1 $2 $3 $4 $5 $6
}

mkdir $P1_DIR

# if C++ skip
#file a.ncexe | grep -i static | grep -i link | grep -i execut
#if [ $? -eq 0 ]; then
#  nm -a a.ncexe | grep __gnu_cxx
#  if [ $? -eq 0 ]; then
#     echo "P1: Statically-linked C++ program detected -- skipping"
#	 exit 1
#  fi
#else
#  ldd a.ncexe | grep "libstdc++"
#  if [ $? -eq 0 ]; then
#     echo "P1: Dynamically-linked C++ program detected -- skipping"
#	 exit 1
#  fi
#fi

# generate coverage info for manually-specified tests (if any)
$PEASOUP_HOME/tools/do_manual_cover.sh

echo "manual cover finished"

# merge all execution traces
touch $EXECUTED_ADDRESSES_FINAL

cat $EXECUTED_ADDRESSES_MANUAL >> $EXECUTED_ADDRESSES_FINAL

echo "Replaying all .json files"
input_cnt=0
REPLAY_DEADLINE=$(( `date +'%s'` + $COVERAGE_REPLAY_TIMEOUT ))
#run all .json inputs through the replayer starting with most recent
for i in `ls -t $CONCOLIC_DIR/*.json`
do
    rm -f exit_status stdout.* stderr.*
    rm -rf grace_replay/

    if [ `date +'%s'` -ge $REPLAY_DEADLINE ]; then
		break
    fi

	#FYI: I Believe this check is in the loop to avoid calling this program if grace completely 
	#failed or was not called at all, but I can't remember. I recommend just leavin it for now.
#check to see if the sym file exists, if not create it.
	if [ ! -e $TOP_LEVEL/a.sym ]; then
		$GRACE_HOME/concolic/src/util/linux/objdump_to_grace $STRATAFIED_BINARY
	fi

    input=`basename $i .json`
    #input_number=`echo $input | sed "s/.*input_//"`
   # abridged_number=`echo $input_number | sed 's/0*\(.*\)/\1/'`

    echo "timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt $STRATAFIED_BINARY $i"

    #generate baseline from the stratafied program, if only to prevent discrepencies when a program prints its own name. 
    #also I want to be as consistent as possible to avoid replayer issues. 
    timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt $STRATAFIED_BINARY $i || continue

	#the exit status file has long been a flag that replay worked, make sure it exists before continuing
    if [ ! -f exit_status ]; then
		continue;
    fi

    status=`cat exit_status | grep "Subject exited" | sed "s/.*status //"`

    #don't consider inputs that cause the program to exit in exit codes 132-140 inclusive
    if [ "$status" -ge 132 ] && [ "$status" -le 140 ]; then
		echo "Ignoring input, bad exit status"
		continue
    fi
    

	#set up the baseline replayer directory, and move all replayer data into the directory
    mkdir $BASELINE_DIR/$input

    mv stdout.$input $BASELINE_DIR/$input/.
    mv stderr.$input $BASELINE_DIR/$input/.
    cat exit_status | grep "Subject exited with status" >tmp
    mv tmp $BASELINE_DIR/$input/exit_status
    mv grace_replay/ $BASELINE_DIR/$input/.

	#---------------Non-Determinism Check-------------------

	#replay again and check for divergence due to non-deterministic output
    echo "timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt --instruction_addresses=$i.coverage $STRATAFIED_BINARY $i"

    timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt --instruction_addresses=$i.coverage $STRATAFIED_BINARY $i || continue

	#create a temporary directory for the second replay values, so I can diff the directory structure.
    mkdir $BASELINE_DIR/${input}_tmp

    mv stdout.$input $BASELINE_DIR/${input}_tmp/.
    mv stderr.$input $BASELINE_DIR/${input}_tmp/.
    cat exit_status | grep "Subject exited with status" >tmp
    mv tmp $BASELINE_DIR/${input}_tmp/exit_status
    mv grace_replay/ $BASELINE_DIR/${input}_tmp/.

    diff -r $BASELINE_DIR/$input $BASELINE_DIR/${input}_tmp
    if [ ! $? -eq 0 ]; then
		echo "Replayer divergence detected for input: $input, rerunning replayer with -r"

		#destroy original baseline directory
		rm -r $BASELINE_DIR/$input
		#clean up the second replay baseline directory
		rm -r $BASELINE_DIR/${input}_tmp

		#rerun the replayer with -r
		#BEN NOTE: It is my understanding that once run with -r, when we actual replay a peasoup'ed version we 
		#do not need to run with -r. I.e., replay is oblivious to whether or not we used -r.
                echo "timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt -r $STRATAFIED_BINARY $i"
		timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt -r $STRATAFIED_BINARY $i || continue

		#recreate a temporary directory for the second replay value, so I can diff the directory structure with baseline.
		mkdir $BASELINE_DIR/${input}

		mv stdout.$input $BASELINE_DIR/${input}/.
		mv stderr.$input $BASELINE_DIR/${input}/.
		cat exit_status | grep "Subject exited with status" >tmp
		mv tmp $BASELINE_DIR/${input}/exit_status
		mv grace_replay/ $BASELINE_DIR/${input}/.

		#Run the replayer one more time without -r, to get a second run to compare against (-r is not necessary once it has been used once)
                echo "timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt --instruction_addresses=$i.coverage $STRATAFIED_BINARY $i"
		timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt --instruction_addresses=$i.coverage $STRATAFIED_BINARY $i || continue

		#create a temporary directory for the second replay values, so I can diff the directory structure.
		mkdir $BASELINE_DIR/${input}_tmp

		mv stdout.$input $BASELINE_DIR/${input}_tmp/.
		mv stderr.$input $BASELINE_DIR/${input}_tmp/.
		cat exit_status | grep "Subject exited with status" >tmp
		mv tmp $BASELINE_DIR/${input}_tmp/exit_status
		mv grace_replay/ $BASELINE_DIR/${input}_tmp/.


		#Check for a divergence between the new baseline using -r and the rerun
		#If a divergence is detected, ignore the input (remove from baseline), and continue
		diff -r $BASELINE_DIR/$input $BASELINE_DIR/${input}_tmp
		if [ ! $? -eq 0 ]; then
			echo "Replayer divergence detected using -r for input: $input, ignoring input"
			#clean up the second replay baseline directory
			rm -r $BASELINE_DIR/${input}_tmp
			#remove original baseline run as well. 
			rm -r $BASELINE_DIR/${input}
			continue
		fi		
    fi #end of non-determinism check
	
	#at this point, the replayer should have produced baseline results that have been validated for non-determinism

	#clean up the second replay baseline directory
	rm -rf $BASELINE_DIR/${input}_tmp

	#increment input counter (counter indicates how many inputs have been validated)
    input_cnt=`expr $input_cnt + 1`
done

rm -f exit_status stdout.* stderr.*
rm -rf grace_replay/

echo "Finished replaying .json files: Replayed $input_cnt inputs"

echo "Choosing at most $INPUT_CUTOFF inputs with best coverage"

if [ "$input_cnt" -ne 0 ]; then
	GREEDY_COVER=`$GRACE_HOME/concolic/scripts/set_cover.py $INPUT_CUTOFF $CONCOLIC_DIR/*.coverage`


	input_cnt=`echo $GREEDY_COVER|wc -w`

	echo "Chose $input_cnt inputs"

	cat $GREEDY_COVER >> $EXECUTED_ADDRESSES_CONCOLIC

# Remove inputs that were not chosen.
	for i in $BASELINE_DIR/*
	do
		if [[ ! "$GREEDY_COVER" =~ `basename $i` ]]; then
			rm -r $i
		fi
	done

fi

touch $EXECUTED_ADDRESSES_CONCOLIC

cat $EXECUTED_ADDRESSES_CONCOLIC >> $EXECUTED_ADDRESSES_FINAL

# sanity filter, keep only well formed addresses
cat $EXECUTED_ADDRESSES_FINAL | sed 's/.*\(0x.*\)/\1/' >tmp
mv tmp $EXECUTED_ADDRESSES_FINAL

sort $EXECUTED_ADDRESSES_FINAL | uniq > tmp
mv tmp $EXECUTED_ADDRESSES_CONCOLIC

# produce coverage file
$PEASOUP_HOME/tools/cover.sh $ORIGINAL_BINARY $MEDS_ANNOTATION_FILE $EXECUTED_ADDRESSES_FINAL $LIBC_FILTER $COVERAGE_FILE $BLACK_LIST

#just in case something went wrong, touch the coverage file. An empty coverage file is permissible, but a missing one will cause PN to crash

touch $COVERAGE_FILE

#execute_pn $CLONE_ID $BED_SCRIPT $LIBC_FILTER $COVERAGE_FILE $P1THRESHOLD $DO_CANARIES

echo "$PN_BINARY --variant_id=$CLONE_ID --bed_script=$BED_SCRIPT --coverage_file=$COVERAGE_FILE --pn_threshold=$P1THRESHOLD --canaries=$DO_CANARIES --blacklist=$LIBC_FILTER --no_p1_validate"

$PN_BINARY --variant_id=$CLONE_ID --bed_script=$BED_SCRIPT --coverage_file=$COVERAGE_FILE --pn_threshold=$P1THRESHOLD --canaries=$DO_CANARIES --blacklist=$LIBC_FILTER --no_p1_validate --shared_object_protection
