#!/bin/bash

if [ $# -lt 1 ]; then
	echo "Usage."
	echo "$0 [start|stop] [...]"
	exit
fi

pidfiles="/tmp/"
directory=""
size=0
monitor_time=0
debug=1
ip=""

task=$1
shift

#only take additional parameters if we are starting.
if [ "$task" == "start" ]; then
	if [ $# -ne 4 ]; then
		echo "$0 start <ip address> <interface> <log directory> <log count>"
		exit;
	fi
	ip=$1
	shift
	interface=$1
	shift
	directory=$1
	shift;
	count=$1
	shift;

	if [ $debug -ne 0 ]; then
		echo "IP: " $ip;
		echo "Directory: " $directory;
		echo "Size: " $size;
		echo "Monitor time: " $monitor_time;
	fi
fi

case "$task" in
	start)
		echo -n "Starting ..."

		if [ ! -d $directory ]; then
			if [ $debug -ne 0 ]; then
				echo "Making $directory"
			fi
			mkdir -p $directory
		fi
		daemon --name "tcpdump" --pidfiles $pidfiles --chdir `pwd` -- tcpdump -C 100 -w $directory/monitor.pcap -p -i $interface -W $count "host $ip"
		echo "done"
		;;
	stop)
		echo -n "Stopping ..."
		daemon --stop --name "tcpdump" --pidfiles $pidfiles
		echo "done"
		;;
	*)
		echo "Invalid task command."
		;;
esac
