#!/bin/bash 

$PS_GREP -i -e " rl " -e " () " -e " IL " $1 > $1.reloc 

