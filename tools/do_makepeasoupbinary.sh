#!/bin/sh 


name=$1

current_dir=`pwd`
peasoup_binary=$name.sh

echo "#!/bin/sh" >> $peasoup_binary
echo "" >> $peasoup_binary

#  setsid made the login() program work, but makes other things more difficult.
# echo "setsid $current_dir/ps_run.sh $current_dir \"\$0\" \"\$@\"" >> $peasoup_binary
#
echo "$current_dir/ps_run.sh $current_dir \"\$0\" \"\$@\"" >> $peasoup_binary
echo "SAVE_EXIT_CODE=\$?" >> $peasoup_binary
echo "datapath=$current_dir" >> $peasoup_binary

cat >> $peasoup_binary <<"EOF"

if [ -f $datapath/diagnostics.out ]; then
	len=`/bin/cat $datapath/diagnostics.out | wc -l` 
	if [ $len -gt 0 ]; then 

        # make output more concise
#		/bin/wc -l $datapath/diagnostics.out
#	    /bin/sort $datapath/diagnostics.out | uniq > tmp.$$
	    /bin/cat $datapath/diagnostics.out | uniq > tmp.$$
		mv tmp.$$ $datapath/diagnostics.out

		#echo "--------------------------------------------------------"
		#echo "-        PEASOUP DETECTED AND CONFINED ERRORS          -"
		#echo "- (and possibly detected that some errors were benign) -"
		#echo "-               (Summarized below)                     -"
		#echo "--------------------------------------------------------"
		#cat $datapath/diagnostics.out
	fi
fi

# final check, in case we couldn't catch the signal
if [ $SAVE_EXIT_CODE = 139 ]; then
	exit 200
fi
exit $SAVE_EXIT_CODE
EOF

chmod +x $peasoup_binary

cp $PEASOUP_HOME/tools/ps_run.sh $current_dir

