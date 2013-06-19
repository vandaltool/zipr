CUMUL_DIAGNOSTICS=$1
BENIGN_ADDRESS_OUTPUT_FILE=$2

# extract addresses from diagnostic file
# look for C1 class of benign errors only (C1: number handling)
cat $CUMUL_DIAGNOSTICS | grep -i diagnos | grep C1 | sed 's/.*diagnosis.*PC:\(.*\)/\1/' | cut -d' ' -f1 | sort | uniq > $BENIGN_ADDRESS_OUTPUT_FILE
