#!/bin/sh

# Don't use this script directly

# need to get these from ps_analyze instead of hardwiring them
STRATA_DOUBLE_FREE=1 STRATA_HEAPRAND=1 STRATA_PC_CONFINE=1 STRATA_PC_CONFINE_XOR=0 STRATA_SIEVE=1 STRATA_RC=1 STRATA_PARTIAL_INLINING=0 STRATA_ANNOT_FILE=../../../a.ncexe.annot ../../../a.stratafied "$@"
status=$?

exit $status
