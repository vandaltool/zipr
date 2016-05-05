#!/bin/bash
for i in `ls`
do
	if [ -d "$i" ]; then
		pushd "$i"
		svn update
		popd
	fi

done
