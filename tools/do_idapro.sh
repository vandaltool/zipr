#!/bin/bash 

analyze_file()
{
	file=$1
	echo STARS Analyzing $file


	#
	# This line is added to turn off screen output to display
	#
	case "$IDAROOT" in
    		*idapro5* )
        		echo "IDA 5.* detected."
        		$SMPSA_HOME/SMP-analyze.sh $file
        		;;
    		*idapro6* )
        		# only works on IDA 6.0+
        		echo "IDA 6.* detected."
        		screen -D -L -ln -m -a -T xterm sh -x $SMPSA_HOME/SMP-analyze.sh $file
        		;;
	esac

	lines=`cat $file.annot | wc -l`

	#
	# simple failure test for idapro
	#
	if [ $lines -lt 10 ]; then
		echo Failed to produce a valid annotations file for $file.
		exit -1 
	fi

}

analyze_file a.ncexe
cd shared_objects

for i in `cat ../shared_libs`; do
	analyze_file $i
done

cd -
exit 0
