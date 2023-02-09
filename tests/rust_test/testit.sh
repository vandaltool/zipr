#!/bin/bash 

testone()
{
	local PUT=$1
	local OPTS=$2
	set -x
	set -e
	rustc $PUT.rs -o $PUT

	$PSZ $PUT ./$PUT.zipr $OPTS
	diff <(./$PUT 2>&1 ) <(./$PUT.zipr 2>&1 )

	# turn off exit-on-error because some programs err and we need their exit code.
	set +e
	./$PUT > /dev/null 2>&1
	local putRes=$?
	./$PUT.zipr > /dev/null 2>&1
	local putResZipr=$?
	set -e

	if [[ $putRes != $putResZipr ]];
	then
		echo failed!
		exit 1
	fi

	rm -rf peasoup* $PUT.zipr $PUT.zipr
}

main()
{
	for bench in hangman 8q hello throw
	do
		for opts in "-c rida" "" 
		do
			testone $bench "$opts"
		done
	done
	exit 0

}

main "$@"
