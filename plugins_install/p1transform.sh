#!/bin/bash

PN_TIMEOUT_VALUE=21600

varid=$1
shift

$PEASOUP_HOME/tools/do_p1transform.sh $varid a.ncexe a.ncexe.annot $PEASOUP_HOME/tools/bed.sh $PN_TIMEOUT_VALUE $*

