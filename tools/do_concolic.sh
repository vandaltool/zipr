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
if [ ! -f $GRACE_HOME/concolic/src/util/linux/objdump_to_grace ]; then  
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
$GRACE_HOME/concolic/src/util/linux/objdump_to_grace $strata_exe
if [ ! -f $sym ]; then
	echo Failed to produce .sym file
	exit 1;
fi

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done


oldulimit=`ulimit -S -t`
ulimit -S -t 61
echo STRATA_GRACE=1 $GRACE_HOME/concolic/src/util/linux/run $extra_args  -s $sym -o replay,coverage,instruction_addresses $strata_exe
     STRATA_GRACE=1 $GRACE_HOME/concolic/src/util/linux/run $extra_args  -s $sym -o replay,coverage,instruction_addresses $strata_exe
ulimit -S -t $oldulimit

echo cleaning up
killall -q controller
killall -q $strata_exe

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done
