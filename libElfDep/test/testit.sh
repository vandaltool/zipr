#!/bin/bash

cleanup()
{
	echo "************"
	echo "test failed."
	echo "************"
	exit 1
}

	
# make sure xforms are built
scons || cleanup

$PSZ /bin/ls ./xxx -c move_globals=on -o move_globals:--elftables -c edt=on || cleanup
 
/bin/ls /tmp || cleanup
./xxx /tmp || cleanup

echo
echo "test passed."
echo
