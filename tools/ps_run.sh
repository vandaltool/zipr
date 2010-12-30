#!/bin/sh

datapath=$1
shift;

STRATA_ANNOT_FILE=$datapath/a.ncexe.annot STRATA_PC_CONFINE=0 $datapath/a.stratafied $*


