#!/bin/bash  -x

# The use of timeout differs depending on the version of linux.
# This is a quick hack to determine how timeout should be envoked
# to send a sigusr1.

alias


time=$1
shift

if $PS_TIMEOUT --help | grep --quiet -- "--signal=SIGNAL"
then

	$PS_TIMEOUT --signal=sigusr1 $time "$@"

else
	$PS_TIMEOUT -10 $time "$@"

fi
