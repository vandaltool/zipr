#!/bin/sh

exe=$1
shift
extra_args=$*
strata_exe=$exe.stratafied
annot=$exe.ncexe.annot

whoami=`whoami`


# get a starting pc
line=`cat $annot|egrep " FUNC GLOBAL main"|sed "s/  */ /g"`
start_ea=`echo $line |cut -d" " -f1`

# get an ending pc
line=`cat $annot|egrep " FUNC GLOBAL exit"|sed "s/  */ /g"`
stop_ea=`echo $line |cut -d" " -f1`

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done
STRATA_GRACE=1 controller $extra_args --start $start_ea --stop $stop_ea $strata_exe 

echo cleaning up
killall -q controller
killall -q $strata_exe

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done
