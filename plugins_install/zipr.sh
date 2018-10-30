#/bin/bash 

cloneid=$1
shift
other_opts="$@"

(set -x ; LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ZIPR_INSTALL/lib $ZIPR_INSTALL/bin/zipr.exe --variant $cloneid --zipr:objcopy $PS_OBJCOPY $other_opts)
