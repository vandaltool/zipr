#!/bin/sh 

grep -i -e " rl " -e " () " $1 > $1.reloc 

