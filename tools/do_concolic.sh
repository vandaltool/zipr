#!/bin/sh

exe=$1
shift
extra_args=$*
strata_exe=$exe.stratafied
annot=$exe.ncexe.annot
sym=$exe.sym

whoami=`whoami`

# 
# simple error checking
# 
if [ $GRACE_HOME"X" = "X" ]; then echo Failed to set GRACE_HOME; exit 2; fi
if [ ! -f $GRACE_HOME/concolic/src/util/linux/meds_annot_to_grace ]; then  
	echo "Failed to set GRACE_HOME properly (i.e. wrong path)"
	exit 3 
fi


# get a starting pc
line=`cat $annot|egrep " FUNC GLOBAL main"|sed "s/  */ /g"`
start_ea=`echo $line |cut -d" " -f1`

# get an ending pc
line=`cat $annot|egrep " FUNC GLOBAL exit"|sed "s/  */ /g"`
stop_ea=`echo $line |cut -d" " -f1`

# assume grace_home env is set.
$GRACE_HOME/concolic/src/util/linux/meds_annot_to_grace $annot
if [ ! -f $sym ]; then
	echo Failed to produce .sym file
	exit 1;
fi

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done
STRATA_GRACE=1 controller $extra_args --start $start_ea --stop $stop_ea --symbols $sym $strata_exe

echo cleaning up
killall -q controller
killall -q $strata_exe

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done
