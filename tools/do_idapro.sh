
#
# This line is added to turn off screen output to display
#
case "$IDAROOT" in
    *idapro5* )
        echo "IDA 5.* detected."
        $SMPSA_HOME/SMP-analyze.sh a.ncexe
        ;;
    *idapro6* )
        # only works on IDA 6.0+
        echo "IDA 6.* detected."
        screen -D -L -ln -m -a -T xterm sh -x $SMPSA_HOME/SMP-analyze.sh a.ncexe
        ;;
esac

lines=`cat a.ncexe.annot | wc -l`

#
# simple failure test for idapro
#
if [ $lines -lt 10 ]; then
	echo Failed to produce a valid annotations file.
	exit -1 
else
	exit 0
fi



