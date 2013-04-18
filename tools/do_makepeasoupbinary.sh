#!/bin/sh 


name=$1

current_dir=`pwd`
peasoup_binary=$name.sh

echo "#!/bin/sh" >> $peasoup_binary
echo "" >> $peasoup_binary
echo "setsid $current_dir/ps_run.sh $current_dir \"\$0\" \"\$@\"" >> $peasoup_binary
chmod +x $peasoup_binary

cp $PEASOUP_HOME/tools/ps_run.sh $current_dir

