#/bin/bash 

cloneid=$1
shift
other_opts="$@"

(set -x ; LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PEASOUP_HOME/zipr_install/lib $PEASOUP_HOME/zipr_install/bin/zipr.exe --variant $cloneid --zipr:objcopy $PS_OBJCOPY $other_opts)
