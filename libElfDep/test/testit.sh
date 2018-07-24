#!/bin/bash

cleanup()
{
	echo test failed.
	exit 1
}
	
# make sure xforms are built
scons || cleanup

$PSZ /bin/ls ./xxx -c move_globals=on -o move_globals:--elftables -c edt=on || cleanup
 
/bin/ls /tmp |tee tmp.out || cleanup
./xxx /tmp |tee edt.out || cleanup

echo test passed.
