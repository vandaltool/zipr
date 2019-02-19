#!/bin/sh

# Demonstration of HeapRand

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

clear
echo "HEAP RANDOMIZATION demonstration"
echo "Sample program: Towers of Hanoi"
Pause
clear

# First, display the program
cat malloc.c |less

Pause
clear
echo "Running malloc.original"
./malloc.original 3

Pause
clear
echo "Running malloc.protected with randomizing log messages on."
Pause
# run program
STRATA_PC_CONFINE=1 STRATA_ANNOT_FILE=malloc.original.annot STRATA_LOG=heaprand STRATA_HEAPRAND=1 ./malloc.protected 3  > output 2>&1

# Show output in pretty form?
cat output |less


