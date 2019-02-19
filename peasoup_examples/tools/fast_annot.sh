#!/bin/sh  -x

ls -l a.ncexe.annot

mv a.ncexe.annot a.ncexe.annot.full
grep "FUNC GLOBAL" a.ncexe.annot.full >  a.ncexe.annot

ls -l a.ncexe.annot

