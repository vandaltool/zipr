#!/bin/bash  -x

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
		$PEASOUP_UMBRELLA_DIR/IdaProServer/SMP-analyze.sh $file $stars_options
		rc=$?

		if [ ! $rc -eq 0 ]; then
			echo "STARS (Remote) failure..."
		fi
	fi

	if [ ! $rc -eq 0 ]; then
		echo "STARS (Local) Analyzing $file target is: $target_name"

		case "$IDAROOT" in
    		*idapro5* )
        		echo "IDA 5.* detected."
        		$SMPSA_HOME/SMP-analyze.sh $file $stars_options
        		;;
    		*idapro6*|*idaproCur*)
        		# only works on IDA 6.0+
        		echo "IDA 6.* detected."
        		#screen -D -L -ln -m -a -T xterm sh -x $SMPSA_HOME/SMP-analyze.sh $file $stars_options
        		TVHEADLESS=1 $SMPSA_HOME/SMP-analyze.sh $file $stars_options
        		;;
		*)
			echo Cannot detect IDA version: $IDAROOT
			exit 1
		esac
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

}

if [ ! -z $1 ]; then
	TARGET_NAME=$1	
else
	TARGET_NAME=a.ncexe
fi

shift

analyze_file a.ncexe $TARGET_NAME $@

if [ -d shared_objects ]; then
	cd shared_objects

	for i in `cat ../shared_libs`; do
		analyze_file $i $i $@
	done

	cd -
fi

exit 0
