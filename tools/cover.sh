#!/bin/sh
#
# ./cover.sh <original_binary> <MEDS annotation_file> <executed_addresses_file> <filter_file> <output_coverage_file> <output_blacklist_file>
#

# inputs
ORIGINAL_BINARY=$1              # a.ncexe
ANNOTATION_FILE=$2              # a.ncexe.annot
EXECUTED_ADDRESS_FILE=$3        # list of executed addresses (e.g. Grace or manual)
FILTER_FILE=$4                  # list of known functions to blacklist, e.g. libc
# outputs
OUTPUT_COVERAGE_FILE=$5         # output file with coverage info per function
OUTPUT_BLACKLIST_FILE=$6        # output file with list of functions to blacklist
# other

CANDIDATE_FNS_PRE_LIBC=`dirname $6`/p1.candidates.prelibc


$SECURITY_TRANSFORMS_HOME/tools/cover/cover $ORIGINAL_BINARY $ANNOTATION_FILE $EXECUTED_ADDRESS_FILE $OUTPUT_COVERAGE_FILE
status=$?
cp $FILTER_FILE $OUTPUT_BLACKLIST_FILE
cat $OUTPUT_COVERAGE_FILE | cut -f1 -d" " > $CANDIDATE_FNS_PRE_LIBC

if [ ! $status -eq 0 ]; then
#	cp $FILTER_FILE $OUTPUT_BLACKLIST_FILE
	return 1
fi

return 0

#below is a relic from when PN could not take ina  coverage file, it is left here
#as it is uncertain as to whether we do want to filter functions at this level
#in this scenario yet.

#
# Prune out functions that do not have sufficient coverage
# Any function whose coverage metric starts with 0.0, e.g. 0.09, 0.0123, is pruned out
# We effectively prune out any functions whose coverage is not at least 10% 
#
# Yes, this is a hack... but it will do for now until we get a more sophisticated definition of coverage
#

#if [ ! -f $OUTPUT_COVERAGE_FILE ]; then
#	cp $FILTER_FILE $OUTPUT_BLACKLIST_FILE
#	return 1
#fi

#cat $OUTPUT_COVERAGE_FILE | cut -f1 -d" ">$CANDIDATE_FNS_PRE_LIBC
#grep -v "0\.000" $OUTPUT_COVERAGE_FILE | cut -f1 -d" " > $CANDIDATE_FNS_PRE_LIBC
#grep  "0\.000" $OUTPUT_COVERAGE_FILE | cut -f1 -d" " > $OUTPUT_BLACKLIST_FILE

# Filter out functions that:
#   1. are not sufficiently covered
#   2. are libc function
#cat $FILTER_FILE >> $OUTPUT_BLACKLIST_FILE
#sort $OUTPUT_BLACKLIST_FILE | uniq > tmp.$$
#mv tmp.$$ $OUTPUT_BLACKLIST_FILE

#return 0
