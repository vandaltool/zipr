#!/bin/sh

P1_DIR=p1.xform
CONCOLIC=concolic.files_a.stratafied_0001

# input
EXECUTED_ADDRESS_FILE=$CONCOLIC/executed_address_list.txt
LIBC_FILTER=$PEASOUP_HOME/tools/p1xform.filter.libc.txt

# output
FILTERED_OUT=$P1_DIR/p1.filtered_out

COVERAGE_FNS=$P1_DIR/p1.coverage
CANDIDATE_FNS_PRE_LIBC=$P1_DIR/p1.candidates.prelibc
FINAL_CANDIDATES=$P1_DIR/p1.candidates

#
# Prune out functions that do not have sufficient coverage
# Any function whose coverage metric starts with 0.0, e.g. 0.09, 0.0123, is pruned out
# We effectively prune out any functions whose coverage is not at least 10% 
#

$SECURITY_TRANSFORMS_HOME/tools/cover/cover a.ncexe a.ncexe.annot $EXECUTED_ADDRESS_FILE $COVERAGE_FNS
grep -v "0\.0" $COVERAGE_FNS | cut -f1 -d" " > $CANDIDATE_FNS_PRE_LIBC
grep  "0\.0" $COVERAGE_FNS | cut -f1 -d" " > $FILTERED_OUT

if [ ! -f $COVERAGE_FNS ]; then
	return 1
fi

# Filter out functions that:
#   1. are not sufficiently covered
#   2. are libc function
cat $LIBC_FILTER >> $FILTERED_OUT
sort $FILTERED_OUT | uniq > tmp.$$
mv tmp.$$ $FILTERED_OUT
rm tmp.$$
