#!/bin/bash 

analyze_file()
{
	file=$1
	target_name=$2
	stars_options=$3

	#
	# This line is added to turn off screen output to display
	#
	rc=1
	if [ ! -z "$IDA_PRO_SERVER_HOST" ]; then
		echo "STARS (Remote on ${IDA_PRO_SERVER_HOST}) Analyzing $file target is: $target_name"
		$PEASOUP_UMBRELLA_DIR/IdaProServer/SMP-analyze-remote.sh $file $stars_options
		rc=$?

		if [ ! $rc -eq 0 ]; then
			echo "STARS (Remote) failure..."
			exit 1
		fi
	fi

	if [ ! $rc -eq 0 ]; then
		echo "STARS (Local) Analyzing $file target is: $target_name"
        	TVHEADLESS=1 $SMPSA_HOME/SMP-analyze.sh $file $stars_options
	fi

	lines=`cat $file.annot | wc -l`

	#
	# simple failure test for idapro
	#
	if [ $lines -lt 10 ]; then
		echo Failed to produce a valid annotations file for $file.
		exit 1 
	fi
	# better test
	grep "ANALYSISCOMPLETED" $file.infoannot > /dev/null 2>&1
	if [ $? != 0 ]; then
		echo MEDS Failed to produce successful exit code for $file.
		exit 2 
	fi

	funcs=$(grep -e "FUNC GLOBAL" -e "FUNC LOCAL" a.ncexe.annot |wc -l)
	namedFuncs=$(grep -e "FUNC GLOBAL" -e "FUNC LOCAL" a.ncexe.annot |grep -v "sub_" |wc -l )
	unnamedFuncs=$(grep -e "FUNC GLOBAL" -e "FUNC LOCAL" a.ncexe.annot |grep "sub_" |wc -l )
	echo "#ATTRIBUTE functions=$funcs"
	echo "#ATTRIBUTE named_functions=$namedFuncs"
	echo "#ATTRIBUTE unnamed_functions=$unnamedFuncs"

}

if [[ ! -z $1 ]]; then
	TARGET_NAME=$1	
else
	TARGET_NAME=a.ncexe
fi

shift

analyze_file a.ncexe $TARGET_NAME $@

if [[ -d shared_objects ]]; then
	cd shared_objects

	for i in `cat ../shared_libs`; do
		analyze_file $i $i $@
	done

	cd -
fi

exit 0
