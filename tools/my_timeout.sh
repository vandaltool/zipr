#!/bin/bash 

# The use of timeout differs depending on the version of linux.
# This is a quick hack to determine how timeout should be envoked
# to send a sigusr1.


time=$1
shift

if timeout --help | grep --quiet -- "--signal=SIGNAL"
then

	timeout --signal=sigusr1 $time "$@"

else
	timeout -10 $time "$@"

fi