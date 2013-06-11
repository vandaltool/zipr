#!/bin/bash

# Uses pin to instrument a program for coverage
# then calls a given test script to acquire coverage
# It is assumed that pin is set up to dump results in itrace.out in a dir
# specified in PIN_RESULTS env var. The results are assumed to accumulate. 
# Pin is assumed to be in a directory PIN_HOME (alse a env var). 
BENCH=$1
TEST_SCRIPT=$2
MANUAL_EXE_ADDRESS_OUTPUT_FILE=$3


PIN_BENCH=`pwd`/pin_bench
COVERAGE_RESULTS_FILE=$PEASOUP_HOME/coverage_results/itrace.out
COVER_SCRIPT=$SECURITY_TRANSFORMS_HOME/tools/cover/cover

echo "manual coverage script"

#clean exe addresses
rm -f $COVERAGE_RESULTS_FILE

#Assuming exe addresses are accumulated by pin in itrace.out located in
#PIN_RESULTS
echo "INSTALL PIN BINARY"
echo "COVERAGE_RESULTS_FILE=$COVERAGE_RESULTS_FILE setarch i386 -RL $PIN_HOME/pin -injection child -t $PIN_HOME/source/tools/ManualExamples/obj-ia32/itraceunique.so -- $BENCH \$@" > $PIN_BENCH

chmod +x $PIN_BENCH

#A test script must take as input a modified bin and the original
#also, the script must accept the -i flag, indicating ignore results
eval $TEST_SCRIPT -i $PIN_BENCH $BENCH

#because I now have an ignore flag, this likely won't occur, but just in case.
if [ $? -ne 0 ]; then

	echo "WARNING: manual coverage failed. The running  manual tests reported failure with coverage instrumentation. Continuing with coverage that was collected."
fi

# sanity filter, keep only well formed addresses
cat $COVERAGE_RESULTS_FILE | sed 's/.*\(0x.*\)/\1/' >tmp
mv tmp $COVERAGE_RESULTS_FILE

cp $COVERAGE_RESULTS_FILE $MANUAL_EXE_ADDRESS_OUTPUT_FILE

exit 0


