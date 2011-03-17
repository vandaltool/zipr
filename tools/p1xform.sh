#!/bin/sh

#
# Argument is the directory created to store the stratafied binary
#
cd $1

$STRATA_REWRITE/tools/transforms/p1transform a.ncexe a.ncexe.annot

