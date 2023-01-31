#!/bin/bash 

testone()
{
	local PUT=$1
	local OPTS=$2
	set -x
	set -e
	go build $PUT.go

	$PSZ $PUT ./$PUT.zipr $OPTS
	diff <(./$PUT 2>&1 ) <(./$PUT.zipr 2>&1 )
	./$PUT > /dev/null 2>&1
	local putRes=$?
	./$PUT.zipr > /dev/null 2>&1
	local putResZipr=$?

	if [[ $putRes != $putResZipr ]];
	then
		echo failed!
		exit 1
	fi

	rm -rf peasoup* $PUT.zipr $PUT.zipr
}

main()
{
	for bench in panic hello 8q
	do
		for opts in "-c rida" "" 
		do
			testone $bench "$opts"
		done
	done
	exit 0

}

main "$@"
