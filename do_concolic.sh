#!/bin/sh

exe=$1

$STRATAFIER/do_stratafy.sh $exe

strata_exe=new.exe

whoami=`whoami`

start_ea=`nm $exe|egrep " main$"|cut -f1 -d" "`

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done
STRATA_GRACE=1 controller --start $start_ea $strata_exe

echo cleaning up
killall -q controller
killall -q $strata_exe

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done
