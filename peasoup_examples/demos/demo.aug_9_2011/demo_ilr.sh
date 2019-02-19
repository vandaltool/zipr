#!/bin/sh

# Author: Michele Co, mc2zk@virginia.edu
#
# Demonstrate operation of Instruction Layout Randomization
# by turning on some Strata log messages

# A pause function
Pause()
{
    key=""
    echo -n "\nPress any key to continue...\n"
    echo
    stty -icanon
    key=`dd count=1 2>/dev/null`
    stty icanon
}

echo "PEASOUP-protected dumbledore on good input"
STRATA_LOG=spri ./dumbledore.protected < dumbledore.good_inputs/good.txt

echo
echo
Pause
clear

echo "PEASOUP-protected dumbledore on malicious input"
STRATA_LOG=spri ./dumbledore.protected < dumbledore.exploits/badB.dynamic.txt

