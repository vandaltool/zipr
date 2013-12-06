#!/bin/sh 

bits=$1
shift

program=$1
find_string_log=$2

# generate string signatures off the binary
$PEASOUP_HOME/tools/generate_string_signatures.sh "$program" "$program.sigs" $find_string_log
cp $program.sigs $program.sigs.orig

# copy application firewall library 
# for now, it's only SQL
cp $SECURITY_TRANSFORMS_HOME/appfw/lib/libappfw.so${bits} libappfw.so

$PEASOUP_HOME/tools/update_env_var.sh DO_APPFW 1
