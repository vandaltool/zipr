#!/bin/bash
IS_UP=1
until [ $IS_UP -eq 0 ]; do
	echo "quit" | telnet $1 $2 2>/dev/null | grep "Connected" >/dev/null
	IS_UP=$?
done

exit 0
