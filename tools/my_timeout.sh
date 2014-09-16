#!/bin/bash  -x

# The use of timeout differs depending on the version of linux.
# This is a quick hack to determine how timeout should be envoked
# to send a sigusr1.

alias


time=$1
shift

if gtimeout --help | ggrep --quiet -- "--signal=SIGNAL"
then

	gtimeout --signal=sigusr1 $time "$@"

else
	gtimeout -10 $time "$@"

fi
