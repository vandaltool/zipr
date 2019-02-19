#!/bin/bash

firstfile="$1"
secondfile="$2"


objdump -d $firstfile > $firstfile.dis
objdump -d $secondfile > $secondfile.dis

sdiff -w 200 $firstfile.dis $secondfile.dis


