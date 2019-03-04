#!/bin/sh 

prog=$1
out=$2

#strings -n 8 $prog | sort  | uniq -i | awk '{ printf "%d:%s\n", length($0), $0;}'  | sort -r -n | sed 's/^[0-9]*://'  > $2
strings -n 8 $prog | sort  | uniq -i > $2

