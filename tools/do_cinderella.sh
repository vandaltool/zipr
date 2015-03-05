#!/bin/bash
#
# pre: we are in the top-level directory created by ps_analyze.sh
#
# @todo:
#     cleanup
#     treat malloc/free differently then the rest of libc.spec
#     better output files for positive/negative inferences
#     fix bug -- something is wrong with positive inference when the fn we're looking for 
#                is not even supported
#     rename a.ncexe.inferfn --> a.ncexe.cinderella
#

TESTABLE=a.ncexe.cinderella
ORIG_ID=$1

$SECURITY_TRANSFORMS_HOME/libIRDB/test/clone.exe $ORIG_ID clone.id
cloneid=`cat clone.id`

echo "New clone id for function inference: $cloneid"

# prep the binary for testing
#   // NO LONGER     pin all functions (no longer pin???)
#     splice-in our testing loop into the target program
$SECURITY_TRANSFORMS_HOME/tools/cinderella/cinderella_prep.exe $cloneid

# get list of all functions
$SECURITY_TRANSFORMS_HOME/tools/cgclibc/display_functions.exe $cloneid | grep "^function" | cut -d' ' -f2 > cinderella.functions.all 

# statically get possible candidates for malloc/free
$SECURITY_TRANSFORMS_HOME/tools/cgclibc/cgclibc.exe $cloneid > cinderella.static.pass1
grep "positive malloc" cinderella.static.pass1 > cinderella.static.pass1.malloc
grep "positive free" cinderella.static.pass1 > cinderella.static.pass1.free

# produce a zipr'd version so that we can dynamically test behavior
# and stash it away
echo "Cinderella: Produce zipr'ed test version: id: $cloneid"
$ZIPR_HOME/src/zipr.exe -v $cloneid -c $ZIPR_INSTALL/bin/callbacks.inferfn.exe -j $PS_OBJCOPY
mv b.out.addseg $TESTABLE

#----------------------------------------------------------
# Dynamically test for a whole bunch of functions
#----------------------------------------------------------
$PEASOUP_HOME/tools/do_prince.sh `pwd`/$TESTABLE $PEASOUP_HOME/tools/cinderella.spec cinderella.functions.all

#
# Look for malloc
#
grep -i "positive malloc" cinderella.static.pass1.malloc | cut -d' ' -f4 > malloc.addresses
$PEASOUP_HOME/tools/do_prince.sh `pwd`/$TESTABLE $PEASOUP_HOME/tools/cinderella.malloc.spec malloc.addresses

#
# Need to find the "true" malloc/free combo
#

#
# Use simple dominator heuristic to whittle down possible malloc/free
#
echo "CINDERELLA PASS2: with restrictions on malloc / turn on --dominator"
$SECURITY_TRANSFORMS_HOME/tools/cgclibc/cgclibc.exe $cloneid --positive-inferences cinderella.inferences.positive --dominator > cinderella.static.pass2
count_malloc=`grep "^static positive malloc" cinderella.static.pass2 | wc -l`
count_free=`grep "^static positive free" cinderella.static.pass2 | wc -l`
grep -i "positive malloc" cinderella.static.pass2 | cut -d' ' -f4 > malloc.true.addresses

echo "CINDERELLA: PASS2: #mallocs: $count_malloc  #frees: $count_free"

#
# Haven't yet found the true malloc/free
# Use simple clustering heuristic
#
if [ "$count_malloc" != "1" ] || [ "$count_free" != "1" ] ; then
	echo "CINDERELLA PASS3: with restrictions on malloc / turn on --dominator"
	$SECURITY_TRANSFORMS_HOME/tools/cgclibc/cgclibc.exe $cloneid --positive-inferences cinderella.inferences.positive --dominator --cluster > cinderella.static.pass3
	grep -i "positive malloc" cinderella.static.pass3 | cut -d' ' -f4 > malloc.true.addresses
	count_malloc=`grep "^static positive malloc" cinderella.static.pass3 | wc -l`
	count_free=`grep "^static positive free" cinderella.static.pass3 | wc -l`
fi

echo "CINDERELLA: PASS3: #mallocs: $count_malloc  #frees: $count_free"

#
# if we pin down malloc and free correctly
# let's look for realloc and/or calloc
#
if [ "$count_malloc" = "1" ];then
	if [ "$count_free" = "1" ];then
		echo "CINDERELLA SUCCESS: true malloc() and free() found"

		#
		# Look for calloc/realloc
		# @todo: We should exclude all functions already discovered here to speed this up
		#
		echo "CINDERELLA SUCCESS: look for realloc"
		$PEASOUP_HOME/tools/do_prince.sh `pwd`/$TESTABLE $PEASOUP_HOME/tools/cinderella.realloc.spec malloc.addresses malloc.true.addresses

		# @todo: fix this, not working at all
		echo "CINDERELLA SUCCESS: look for calloc"
		$PEASOUP_HOME/tools/do_prince.sh `pwd`/$TESTABLE $PEASOUP_HOME/tools/cinderella.calloc.spec malloc.addresses malloc.true.addresses
	fi
fi

exit 0
