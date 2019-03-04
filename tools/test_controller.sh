#!/bin/sh

whoami=`whoami`

echo Building files...
gcc -w concolic_test_handshake.c -o test_controller.exe 
gcc -w hanoi_overrun.c -o hanoi_overrun.ncexe
$STRATAFIER/do_stratafy.sh hanoi_overrun.ncexe >/dev/null 2>&1 
mv new.exe hanoi_overrun.stratafied

echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done
STRATA_GRACE=1 STRATA_LOG=ipc ./test_controller.exe ./hanoi_overrun.stratafied

echo cleaning up
killall hanoi_overrun.stratafied
killall test_controller.exe
echo  Removing all ipc queues.
for i in `ipcs -q|grep $whoami |cut -d" " -f 2`; 
do
	ipcrm -q $i
done

rm test_controller
