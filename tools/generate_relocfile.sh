#!/bin/sh 

ggrep -i -e " rl " -e " () " $1 > $1.reloc 

