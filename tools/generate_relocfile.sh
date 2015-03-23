#!/bin/bash 

$PS_GREP -i -e " rl " -e " () " $1 > $1.reloc 

