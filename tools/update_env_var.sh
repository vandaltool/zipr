#!/bin/sh 

var=$1
value=$2

cat ps_run.sh |sed "s/$1=.*$/$1=$2/" > ps_run.sav
mv ps_run.sav ps_run.sh
chmod +x ps_run.sh
