#!/bin/sh 


name=$1

current_dir=`pwd`
peasoup_binary=$name.sh

echo "#!/bin/sh" >> $peasoup_binary
echo "" >> $peasoup_binary
echo "$PEASOUP_HOME/tools/ps_run.sh $current_dir \"\$@\"" >> $peasoup_binary
chmod +x $peasoup_binary

