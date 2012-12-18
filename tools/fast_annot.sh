#!/bin/sh 

mv a.ncexe.annot a.ncexe.annot.full
grep "FUNC GLOBAL" a.ncexe.annot.full >  a.ncexe.annot

