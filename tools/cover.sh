#!/bin/sh

P1_DIR=p1.xform
CONCOLIC=concolic.files_a.stratafied_0001

COVERAGE_FNS=$P1_DIR/p1.coverage
CANDIDATE_FNS_PRE_LIBC=$P1_DIR/p1.candidates.prelibc
FILTERED_OUT=$P1_DIR/p1.fn_coverage.filtered_out
FINAL_CANDIDATES=$P1_DIR/p1.candidates
FINAL_XFORM_FNS=$P1_DIR/p1.final
EXECUTED_ADDRESS_FILE=$CONCOLIC/executed_address_list.txt
LIBC_FILTER=$PEASOUP_HOME/tools/p1xform.filter.libc.txt

#
# Prune out functions that do not have sufficient coverage
# Any function whose coverage metric starts with 0.0, e.g. 0.09, 0.0123, is pruned out
# We effectively prune out any functions whose coverage is not at least 10% 
#
$SECURITY_TRANSFORMS_HOME/tools/cover/cover a.ncexe a.ncexe.annot $EXECUTED_ADDRESS_FILE $COVERAGE_FNS
grep -v "0\.0" $COVERAGE_FNS | cut -f1 -d" " > $CANDIDATE_FNS_PRE_LIBC
grep  "0\.0" $COVERAGE_FNS | cut -f1 -d" " > $FILTERED_OUT

# Prune out libc functions
$PEASOUP_HOME/tools/p1xform.filter.sh $CANDIDATE_FNS_PRE_LIBC $LIBC_FILTER > $FINAL_CANDIDATES

