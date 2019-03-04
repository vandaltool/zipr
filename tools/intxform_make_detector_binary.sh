#!/bin/sh 

name=$1

current_dir=`pwd`
#intxform_fp_detect_binary=$name.sh
intxform_fp_detect_binary=$name

echo "#!/bin/sh" >> $intxform_fp_detect_binary
echo "" >> $intxform_fp_detect_binary
echo "setsid $current_dir/intxform_run.sh $current_dir \"\$0\" \"\$@\"" >> $intxform_fp_detect_binary
echo "SAVE_EXIT_CODE=\$?" >> $intxform_fp_detect_binary
echo "datapath=$current_dir" >> $intxform_fp_detect_binary

cat >> $intxform_fp_detect_binary <<"EOF"

if [ -f $datapath/diagnostics.out ]; then
	len=`cat $datapath/diagnostics.out | wc -l` 
	if [ $len -gt 0 ]; then 

        # make output more concise
	    sort $datapath/diagnostics.out | uniq >> $datapath/diagnostics.cumul.out
	fi
fi

# final check, in case we couldn't catch the signal
if [ $SAVE_EXIT_CODE = 139 ]; then
	exit 200
fi
exit $SAVE_EXIT_CODE
EOF

chmod +x $intxform_fp_detect_binary

cp $PEASOUP_HOME/tools/intxform_run.sh $current_dir


