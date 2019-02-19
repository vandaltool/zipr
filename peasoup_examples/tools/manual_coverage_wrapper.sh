#!/bin/bash

# Uses pin to instrument a program for coverage
# then calls a given test script to acquire coverage
# It is assumed that pin is set up to dump results in itrace.out in a dir
# specified in PIN_RESULTS env var. The results are assumed to accumulate. 
# Pin is assumed to be in a directory PIN_HOME (alse a env var). 
BENCH=$1
TEST_SCRIPT=$2
MANUAL_EXE_ADDRESS_OUTPUT_FILE=$3
#A coverage round that takes longer than 10 minutes is assumed to be an infinite loop.
TIMEOUT_VALUE=600

PIN_BENCH=`pwd`/pin_bench
COVERAGE_RESULTS_DIR=`pwd`/manual_coverage_results/
ACCUMULATED_COVERAGE_FILE=$COVERAGE_RESULTS_DIR/manual_coverage_results.out
COVER_SCRIPT=$SECURITY_TRANSFORMS_HOME/bin/cover

echo "manual coverage script"

#make the coverage results directory, it should exist before hand
#if it does, this could skew the results of this run, so first delete
#the directory, as precaution, then make the dir. 
rm -rf $COVERAGE_RESULTS_DIR
mkdir $COVERAGE_RESULTS_DIR

#Assuming exe addresses are accumulated by pin in itrace.out located in
#PIN_RESULTS
echo "INSTALL PIN BINARY"
echo "COVERAGE_RESULTS_DIR=$COVERAGE_RESULTS_DIR setarch i386 -RL $PIN_HOME/pin -injection child -t $PIN_HOME/source/tools/ManualExamples/obj-ia32/itraceunique.so -- $BENCH \$@" > $PIN_BENCH

chmod +x $PIN_BENCH

#A test script must take as input a modified bin and the original
#also, the script must accept the -i flag, indicating ignore results
eval $TEST_SCRIPT -i $PIN_BENCH $BENCH

#because I now have an ignore flag, this likely won't occur, but just in case.
if [ $? -ne 0 ]; then

	echo "ERROR: manual coverage failed. The running  manual tests reported failure with coverage instrumentation. Ignoring coverage results."

	exit 1
fi

itrace_files=$COVERAGE_RESULTS_DIR/itrace.*

touch $ACCUMULATED_COVERAGE_FILE

for file in $itrace_files
do
	cat $file >>$ACCUMULATED_COVERAGE_FILE
done

# sanity filter, keep only well formed addresses
cat $ACCUMULATED_COVERAGE_FILE | sed 's/\(.*0x.*\)/\1/' >tmp
mv tmp $ACCUMULATED_COVERAGE_FILE

cp $ACCUMULATED_COVERAGE_FILE $MANUAL_EXE_ADDRESS_OUTPUT_FILE

exit 0


