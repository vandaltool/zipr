#!/bin/sh

if [ "${PEASOUP_HOME}x" = "x" ]; then
	echo "environment variable: PEASOUP_HOME is empty."
	echo "PEASOUP_HOME must be defined to point at a valid peasoup_examples directory"
	exit 1
fi

if [ "${STRATA}x" = "x" ]; then
	echo "environment variable: STRATA is empty. "
	echo "STRATA must be defined to point at a valid Strata security branch"
	exit 1
fi

# if the executable hasn't been built, then build it!
if [ ! -f dumbledore.original ]; then
	make dumbledore.original
fi

TOOLBASE=${PEASOUP_HOME}/tools

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

# N.B. - Assumes that dumbledore.original has already been built.
# clear the screen
clear
# 1) Run dumbledore_cmd.original with good input
echo "Running dumbledore.original with a non-malicious input\n\n"
good_input=`cat dumbledore.good_inputs/good.txt`
echo "Input: ${good_input}\n"

echo "./dumbledore.original < dumbledore.good_inputs/good.txt\n"

Pause

./dumbledore.original  < dumbledore.good_inputs/good.txt

Pause

# clear screen before doing the next step
clear

# 2) Run dumbledore.original with a bad input that PEASOUP can catch
#	input is too long and overwrites return address and base pointer

echo "Running dumbledore.original with CODE INJECTION input.\nReported grade will be changed from D to A.\n"

bad_input=`cat dumbledore.exploits/badA.dynamic.txt`
echo "Input: ${bad_input}\n\n"
echo "./dumbledore.original < dumbledore.exploits/badA.dynamic.txt\n"

Pause

./dumbledore.original < dumbledore.exploits/badA.dynamic.txt

Pause
clear

# 3) Run dumbledore.protected on bad input A, show defeat of exploit
echo "At 6-month review: Instruction Set Randomization"
echo "Running PEASOUP-protected dumbledore on CODE INJECTION input.\n"
echo "PEASOUP detects the code injection.\n" 

bad_input=`cat dumbledore.exploits/badA.dynamic.txt`
echo "Input: ${bad_input}\n\n"
echo "./dumbledore.protected < dumbledore.exploits/badA.dynamic.txt\n"

Pause

./dumbledore.protected < dumbledore.exploits/badA.dynamic.txt

Pause
# clear the screen
clear

# 4) Run dumbledore.original on ARC INJECTION input
echo "Running dumbledore.original with ARC INJECTION input\n"
echo "Input will cause username check to be bypassed."
echo "Reported grade will be B, instead of the expected D.\n"
bad_input=`cat dumbledore.exploits/badB.dynamic.txt`
echo  "Input: ${bad_input}\n\n"
echo "./dumbledore.original < dumbledore.exploits/badB.dynamic.txt\n"

Pause

./dumbledore.original < dumbledore.exploits/badB.dynamic.txt

Pause 
clear

# 5) Run dumbledore.protected on bad input #2, show that we did not defeat the exploit
echo "NEW since 6-month review: Instruction Layout Randomization"
echo "Running PEASOUP-protected dumbledore with arc injection attack input\n\n"
badBinput=`cat dumbledore.exploits/badB.dynamic.txt`
echo "Input: ${badBinput}\n\n"
echo "./dumbledore.protected < dumbledore.exploits/badB.dynamic.txt\n"

Pause

./dumbledore.protected < dumbledore.exploits/badB.dynamic.txt


Pause
clear
# 6) Run heap overflow original on some sample inputs
echo "Heap overflow example: non-malicious inputs"
echo "Program takes a filename as an argument and cat's the file."
echo "Input: sample.txt\n\n"
echo "./heap_overflow.original sample.txt\n"

Pause

./heap_overflow.original sample.txt

Pause
clear

echo "Heap overflow example: another non-malicious input"
echo "Program detects disallowed user input and prints error message."
echo "Input: /etc/passwd\n\n"
echo "./heap_overflow.original /etc/passwd\n"

Pause

./heap_overflow.original /etc/passwd


Pause
clear

echo "Heap overflow example: malicious input"
echo "Overflow heap buffer to try and leak sensitive information."
echo "Input: 012345678901234567890123/etc/passwd\n\n"
echo "./heap_overflow.original 012345678901234567890123/etc/passwd\n"

Pause

./heap_overflow.original 012345678901234567890123/etc/passwd


Pause
clear

# 7) Run PS_analyzed heap overflow
echo "NEW since 6-month review: Heap Randomization"
echo "Running PEASOUP-protected heap_overflow on malicious input"
echo "Execution is altered from original.  Information not leaked."
echo "Input: 012345678901234567890123/etc/passwd\n\n"
echo "./heap_overflow.protected 012345678901234567890123/etc/passwd\n"

Pause

./heap_overflow.protected 012345678901234567890123/etc/passwd

echo
echo
echo "END of DEMO"
