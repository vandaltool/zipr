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
P1THRESHOLD=0.50


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

# merge all execution traces
touch $EXECUTED_ADDRESSES_FINAL

cat $EXECUTED_ADDRESSES_MANUAL >> $EXECUTED_ADDRESSES_FINAL

echo "Replaying all .json files"
input_cnt=0
#run all .json inputs through the replayer
for i in `ls $CONCOLIC_DIR/*.json`
do
    rm -f exit_status stdout.* stderr.*
    rm -rf grace_replay/

    if [ $input_cnt -ge $INPUT_CUTOFF ]; then
	break
    fi

    #check to see if the sym file exists, if not create it.
    if [ ! -e $TOP_LEVEL/a.sym ]; then
	$GRACE_HOME/concolic/src/util/linux/objdump_to_grace $STRATAFIED_BINARY
    fi

    input=`basename $i .json`
    #input_number=`echo $input | sed "s/.*input_//"`
   # abridged_number=`echo $input_number | sed 's/0*\(.*\)/\1/'`

    echo "timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt --instruction_addresses=tmp_coverage $STRATAFIED_BINARY $i"

    #generate baseline from the stratafied program, if only to prevent discrepencies when a program prints its own name. 
    #also I want to be as consistent as possible to avoid replayer issues. 
    timeout $REPLAYER_TIMEOUT "$GRACE_HOME/concolic/bin/replayer" --timeout=$REPLAYER_TIMEOUT --symbols=$TOP_LEVEL/a.sym --stdout=stdout.$input --stderr=stderr.$input --logfile=exit_status --engine=sdt --instruction_addresses=tmp_coverage $STRATAFIED_BINARY $i || continue

	#the exit status file has long been a flag that replay worked, make sure it exists before continuing
    if [ ! -f exit_status ]; then
	continue;
    fi

    status=`cat exit_status | grep "Subject exited" | sed "s/.*status //"`

    #don't consider inputs that cause the program to exit in exit coes
    #132-140 inclusive
    if [ $status -ge 132 ] && [ $status -le 140 ]; then
	continue
    fi
    
    mkdir $BASELINE_DIR/$input

    mv stdout.$input $BASELINE_DIR/$input/.
    mv stderr.$input $BASELINE_DIR/$input/.
    cat exit_status | grep "Subject exited with status" >tmp
    mv tmp $BASELINE_DIR/$input/exit_status
    mv grace_replay/ $BASELINE_DIR/$input/.

	#add to coverage total
    cat tmp_coverage >> $EXECUTED_ADDRESSES_CONCOLIC 	

    input_cnt=`expr $input_cnt + 1`
done

rm -f exit_status stdout.* stderr.*
rm -rf grace_replay/


echo "Finished replaying .json files: Replayed $input_cnt inputs"

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

echo "$PN_BINARY --variant_id=$CLONE_ID --bed_script=$BED_SCRIPT --coverage_file=$COVERAGE_FILE --pn_threshold=$P1THRESHOLD --canaries=$DO_CANARIES --blacklist=$LIBC_FILTER"

$PN_BINARY --variant_id=$CLONE_ID --bed_script=$BED_SCRIPT --coverage_file=$COVERAGE_FILE --pn_threshold=$P1THRESHOLD --canaries=$DO_CANARIES --blacklist=$LIBC_FILTER --no_p1_validate
exit 0
