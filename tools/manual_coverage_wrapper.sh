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
EXE_ADDRESSES=$PIN_RESULTS/itrace.out
COVER_SCRIPT=$SECURITY_TRANSFORMS_HOME/tools/cover/cover

echo "manual coverage script"

#clean exe addresses
rm -f $EXE_ADDRESSES

#Assuming exe addresses are accumulated by pin in itrace.out located in
#PIN_RESULTS
echo "INSTALL PIN BINARY"
echo "setarch i386 -RL $PIN_HOME/pin -injection child -t $PIN_HOME/itraceunique.so -- $BENCH \$@" > $PIN_BENCH

chmod +x $PIN_BENCH

#A test script must take as input a modified bin and the original
#also, the script must accept the -i flag, indicating ignore results
eval $TEST_SCRIPT -i $PIN_BENCH $BENCH

#because I now have an ignore flag, this likely won't occur, but just in case.
if [ $? -ne 0 ]; then

	echo "WARNING: manual coverage failed. The running  manual tests reported failure with coverage instrumentation. Continuing with coverage that was collected."
fi

# sanity filter, keep only well formed addresses
cat $EXE_ADDRESSES | sed 's/.*\(0x.*\)/\1/' >tmp
mv tmp $EXE_ADDRESSES

cp $EXE_ADDRESSES $MANUAL_EXE_ADDRESS_OUTPUT_FILE

exit 0


