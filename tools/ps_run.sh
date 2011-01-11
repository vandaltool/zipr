#!/bin/sh

datapath=$1
shift;

STRATA_ANNOT_FILE=$datapath/a.ncexe.annot STRATA_PC_CONFINE=1 $datapath/a.stratafied $*


