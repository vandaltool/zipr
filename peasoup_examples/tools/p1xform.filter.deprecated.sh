#!/bin/sh
#
# Filter out functions we know we don't want to P1 transform
#
# The common use case is to filter out libc functions
#
# $1 is a file containing a list of potential functions to transforms
# $2 is a file containing a list of functions not to transforms, e.g., names of libc functions
#
# Output: 
#   (stdout) list of functions in $1 that are not in $2
#

CANDIDATE_FUNCTIONS=$1
FILTER_SET=$2

while read fn;
do
  grep "^$fn$" $FILTER_SET >/dev/null 2>/dev/null
  if [ ! $? -eq 0 ]; then
    echo $fn
  fi
done < $CANDIDATE_FUNCTIONS

