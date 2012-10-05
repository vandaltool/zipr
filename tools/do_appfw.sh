#!/bin/sh 

program=$1

# generate string signatures off the binary
$PEASOUP_HOME/tools/generate_string_signatures.sh "$program" "$program.sigs"

# copy application firewall library 
# for now, it's only SQL
cp $SECURITY_TRANSFORMS_HOME/appfw/lib/libappfw.so .
