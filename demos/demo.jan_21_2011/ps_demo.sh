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
	make
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
echo "Running dumbledore.original with a good input\n"
good_input=`cat dumbledore.good_inputs/good.txt`
echo "Input: ${good_input}\n"

echo "./dumbledore.original < dumbledore.good_inputs/good.txt\n"

Pause

./dumbledore.original  < dumbledore.good_inputs/good.txt

Pause

# clear the screen
clear

# 2) Run dumbledore_cmd.original with a bad input that PEASOUP can catch
#	input is too long and overwrites return address and base pointer

echo "Running dumbledore.original with a bad input which performs code injection. \n"

bad_input=`cat dumbledore.exploits/badA.txt`
echo "Input: ${bad_input}\n"
echo "./dumbledore.original < dumbledore.exploits/badA.txt\n"

Pause

./dumbledore.original < dumbledore.exploits/badA.txt

Pause

# 3) Run ps_analyze.sh dumbledore_cmd.original dumbledore_cmd.protected
#	Point out IDA pass
#	Point out GraCE run

# clear the screen
clear
echo "Running PEASOUP analysis phase...\n"
echo "${TOOLBASE}/ps_analyze.sh dumbledore_cmd.original dumbledore_cmd.protected\n"

Pause

${TOOLBASE}/ps_analyze.sh dumbledore_cmd.original dumbledore_cmd.protected

Pause

clear
echo "GraCE finds interesting test input which causes infinite loop\n"
echo "Input: \340^A@@^F*nj^B^D.^Az^\b^H^A^BB^B^P^Z^F ^P"

echo
Pause

# test the infinite looping input
sh test_infinite.sh

Pause
# clear the screen
clear

# 4) Run dumbledore.protected on good input
echo "Running dumbledore.protected on good input\n"
echo "Input: ${good_input}\n"
echo "./dumbledore.protected < dumbledore.good_inputs/good.txt\n"

Pause

./dumbledore.protected < dumbledore.good_inputs/good.txt

Pause

# clear the screen
clear
# 5) Run dumbledore.protected on bad input A, show defeat of exploit
echo "Running dumbledore.protected on bad input which performs code injection\n"

input=`cat dumbledore.exploits/badA.txt`
echo "Input: ${bad_input}\n"
echo "./dumbledore.protected < dumbledore.exploits/badA.txt\n"

Pause

./dumbledore.protected < dumbledore.exploits/badA.txt

Pause

# 6) Demonstrate add_pc_confinement.sh
# add_pc_confinement.sh needs to have a second copy of the exe for some reason
cp dumbledore.original tmp

# clear the screen
clear
echo "Program shepherding.  Adding confinement information to the binary.\n"
bash ${STRATA}/tools/pc_confinement/add_confinement_section.sh dumbledore.original tmp | egrep -v EOF

echo
echo

#Pause

#clear
#echo "GDB step through...."
# 7) Run dumbledore.protected in gdb with bad input with bp set at confined_targ_fetch(), fetching a good instruction, and show it when catching the bad instruction.


#Pause
#clear

# 8) Run dumbledore.protected on bad input #2, show that we did not defeat the exploit
#echo "Running dumbledore.protected with arc injection attack input\n"
#badBinput=`cat dumbledore.exploits/badB.txt`
#echo "Input: $badBinput\n\n"

#Pause

#./dumbledore.protected < dumbledore.exploits/badB.txt

