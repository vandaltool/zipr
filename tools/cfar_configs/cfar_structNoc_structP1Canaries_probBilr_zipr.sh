#!/bin/bash 

echo NOC+Bilr generates working binaries, but Bilr is not yet applied.  Avoid this config for now.
exit 1

$PEASOUP_HOME/tools/cfar.sh "$@" --backend zipr --structured_noc --step-option zipr:"--large_only:on true" --structured_p1_canaries  --step p1transform=on --config_name $(basename $0 .sh|sed "s/cfar_//")


# --step-option zipr:"--zipr:seed $$" 
