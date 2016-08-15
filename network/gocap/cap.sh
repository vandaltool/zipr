#!/bin/bash

if [ $# -lt 1 ]; then
	echo "Usage."
	echo "$0 [start|stop] [...]"
	exit
fi

pidfiles="/tmp/"

task=$1
shift

case "$task" in
	start)
		echo -n "Starting ..."
		daemon --name "cap" --pidfiles $pidfiles --chdir `pwd` -- /home/team/gocap//cap -e -d -dst_host 192.168.1.30 -src_host 192.168.1.40 -online eth0  
		echo "done"
		;;
	stop)
		echo -n "Stopping ..."
		daemon --stop --name "cap" --pidfiles $pidfiles
		echo "done"
		;;
	*)
		echo "Invalid task command."
		;;
esac
