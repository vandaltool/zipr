#!/bin/sh

infile=$1
annotfile=$2

for ifunc in `$PS_NM  $infile|grep " i "|cut -f3 -d" "`
do
	cat $annotfile|sed "s/ FUNC GLOBAL $ifunc / FUNC GLOBAL $ifunc IFUNC /" > $annotfile.tmp.$$
	mv $annotfile.tmp.$$ $annotfile
done
