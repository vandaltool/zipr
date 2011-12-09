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

# configuration variables
P1_DIR=p1.xform
CONCOLIC_DIR=concolic.files_a.stratafied_0001
EXECUTED_ADDRESSES=$CONCOLIC_DIR/executed_address_list.txt
EXECUTED_ADDRESSES_MANUAL=manual_coverage.txt
EXECUTED_ADDRESSES_FINAL=final.coverage.txt
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt
BLACK_LIST=$P1_DIR/p1.filtered_out                                            # list of functions to blacklist
COVERAGE_FILE=$P1_DIR/p1.coverage
P1THRESHOLD=0.30

PN_BINARY=$SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe

echo "P1: transforming binary: cloneid=$CLONE_ID bed_script=$BED_SCRIPT timeout_value=$TIMEOUT_VALUE"

execute_pn()
{
	echo "P1: issuing command: $SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe $1 $2 $3 $4 $5 with timeout value=$TIMEOUT_VALUE"

	# On timeout send SIGUSR1 (signal #10)
	timeout -10 $TIMEOUT_VALUE $PN_BINARY $1 $2 $3 $4 $5
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
cat $EXECUTED_ADDRESSES >> $EXECUTED_ADDRESSES_FINAL

# sanity filter, keep only well formed addresses
cat $EXECUTED_ADDRESSES_FINAL | sed 's/.*\(0x.*\)/\1/' >tmp
mv tmp $EXECUTED_ADDRESSES_FINAL

# produce coverage file
$PEASOUP_HOME/tools/cover.sh $ORIGINAL_BINARY $MEDS_ANNOTATION_FILE $EXECUTED_ADDRESSES_FINAL $LIBC_FILTER $COVERAGE_FILE $BLACK_LIST

baseline_flag=0
#delete the coverage file if all baseline exit status results are 139
for i in `ls $CONCOLIC_DIR/input*.json`
do
  input=`basename $i .json`
  input_number=`echo $input | sed "s/input_//"`

  abridged_number=`echo $input_number | sed 's/0*\(.*\)/\1/'`

  #if there is no exit code for the input number, skip for now.
  if [ ! -f "$CONCOLIC_DIR/exit_code.run_$abridged_number.log" ]; then
      echo "do_p1transform.sh: No baseline data for input $input_number, missing exit status"
      continue;
  fi
  echo "Exit status baseline file: $CONCOLIC_DIR/exit_code.run_$abridged_number.log"
  #if baseline exited with 139, ignore input
  grep 139 $CONCOLIC_DIR/exit_code.run_$abridged_number.log
  if [ $? -eq 0 ]; then
      echo "Baseline exit status was 139 for input $i, ignoring input"
      continue
  fi

  echo "do_p1transform.sh: Found at least one grace produced valid baseline comparison output"
  #at this point we know we have baseline data to compare against
  #we assume that if exit status was produced, baseline information is available
  #if I find one, break the loop
  baseline_flag=`expr $baseline_flag + 1`
  break
done

#if no baseline comparison output exists, delete the coverage file
if [ $baseline_flag -eq 0 ]; then
    echo "do_p1transform.sh: no valid baseline comparison output, deleting coverage file"
    rm $COVERAGE_FILE
    touch $COVERAGE_FILE
fi

#just in case something went wrong, touch the coverage file. An empty coverage file is permissible, but a missing one will cause PN to crash

touch $COVERAGE_FILE

if [ $? -eq 0 ]; then
	if [ -f $COVERAGE_FILE ]; then
	    execute_pn $CLONE_ID $BED_SCRIPT $BLACK_LIST $COVERAGE_FILE $P1THRESHOLD $TIMEOUT_VALUE
	else
		echo "No coverage file -- do not attempt P1 transform" > p1transform.out
		exit 1
	fi
else
    execute_pn $CLONE_ID $BED_SCRIPT $LIBC_FILTER $COVERAGE_FILE $P1THRESHOLD $TIMEOUT_VALUE
fi

exit 0
