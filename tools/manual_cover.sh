#!/bin/sh

# Coverage tool for manual tests
#
# Usage:
#   manual_cover.sh <coverageOutputFile> -- cmd arg1 .. argn
#
# Example:
#   manual_cover.sh test1.executed_addresses.txt -- ls -lt .

# If you don't have a coverage tool: uncomment below
# exit 1

# Otherwise, compute coverage below
OUTPUT_FILE=$1
shift
shift

FULL_COMMAND=$*

# We use pin for extracting coverage info
setarch i386 -RL $PEASOUP_HOME/tools/pin/pin -t $PEASOUP_HOME/tools/pin/itraceunique.so -- $FULL_COMMAND

cat itrace.out | grep -v eof > $OUTPUT_FILE
