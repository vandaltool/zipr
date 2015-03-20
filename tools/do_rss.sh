#!/bin/bash  -x


#
# This env. var tells Strata to insert RSS-ing.
# However, we're doing the RSSing via SPRI/IRDB.
# So we need to leave this env. var off.
#
# $PEASOUP_HOME/tools/update_env_var.sh STRATA_SHADOW_STACK 1

$SECURITY_TRANSFORMS_HOME/tools/ret_shadow_stack/ret_shadow_stack.exe $*




