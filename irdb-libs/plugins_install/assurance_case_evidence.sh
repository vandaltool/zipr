#!/bin/bash -x

# check for the logs directory
if [ -d logs ]; then
	# Gather the relevant lines and print to stdout
	grep -h ATTRIBUTE logs/* | grep :: | sed "s/# ATTRIBUTE//g"

else
	echo "ERROR: assurance_case_evidence: no logs directory."
	exit 1
fi

exit 0
