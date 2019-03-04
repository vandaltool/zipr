#!/bin/sh

for i in $1/peasoup_executable_dir*; do
	$PEASOUP_HOME/tools/ps_release.sh $i
done

