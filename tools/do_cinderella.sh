#!/bin/bash
#
# pre: we are in the top-level directory created by ps_analyze.sh
#
#     fix bug -- something is wrong with positive inference when the fn we're looking for 
#                is not even supported
#
# Find possible libc functions in CGC binaries 
#     specified in $LIBC_SEARCH_SPEC
#

ORIG_VARIANT_ID=$1
TESTABLE=a.ncexe.cinderella     

LIBC_SEARCH_SPEC=$PEASOUP_HOME/tools/cinderella.spec

$SECURITY_TRANSFORMS_HOME/libIRDB/test/clone.exe $ORIG_VARIANT_ID clone.id
cloneid=`cat clone.id`

TRUE_MALLOC=malloc.true.functions

# prep the binary for testing
#     pin all functions
#     splice-in our testing loop into the target program
$SECURITY_TRANSFORMS_HOME/tools/cinderella/cinderella_prep.exe $cloneid

# get list of all functions in binary
# for stripped binary, this will typically be of the form:
#    sub_80004fde
$SECURITY_TRANSFORMS_HOME/tools/cgclibc/display_functions.exe $cloneid | grep "^function" | cut -d' ' -f2 > cinderella.functions.all 

# pass 1: statically get possible candidates for malloc/free
#$SECURITY_TRANSFORMS_HOME/tools/cgclibc/cgclibc.exe $ORIG_VARIANT_ID > cinderella.static.pass1
#grep "positive malloc" cinderella.static.pass1 > cinderella.static.pass1.malloc
#grep "positive free" cinderella.static.pass1 > cinderella.static.pass1.free

# produce a zipr'd version so that we can dynamically test behavior
echo "Cinderella: Produce zipr'ed test version: id: $cloneid"
$ZIPR_INSTALL/bin/zipr.exe -v $cloneid -c $ZIPR_INSTALL/bin/callbacks.inferfn.exe -j $PS_OBJCOPY
mv b.out.addseg $TESTABLE

#----------------------------------------------------------------
# We now have a Zipr'd binary in which we inserted a testing loop
# Dynamically test for libc functions
#----------------------------------------------------------------

# Look for potential libc functions in the binary
# TODO: fixme: specify output inference file here
$PEASOUP_HOME/tools/do_prince.sh $cloneid `pwd`/$TESTABLE $LIBC_SEARCH_SPEC cinderella.functions.all

echo "CINDERELLA TODO: rename all libc functions detected: prepend to cinderella namespace, i.e., cinderella::strcpy, cinderella::memcpy"

#
# Look for the true malloc
#
#grep -i "positive malloc" cinderella.static.pass1.malloc | cut -d' ' -f4 > malloc.addresses
#$PEASOUP_HOME/tools/do_prince.sh $cloneid `pwd`/$TESTABLE $PEASOUP_HOME/tools/cinderella.malloc.spec malloc.addresses
#
#
# At this point, we have found a whole bunch of libc functions via
# dynamic testing
#
# Chances are, we have more than one choice for malloc(), so look
# for the true malloc()
#

TMP=tmp.$$

echo "CINDERELLA PASS1: simply intersects static + dynamic"
$SECURITY_TRANSFORMS_HOME/tools/cgclibc/cgclibc.exe $ORIG_VARIANT_ID --positive-inferences cinderella.inferences.positive --negative-inferences cinderella.inferences.negative > cinderella.static.pass1
count_malloc=`grep "^static positive malloc" cinderella.static.pass1 | wc -l`
if [ "$count_malloc" = "0" ]; then
	echo "No dynamic memory allocation in this program"
	exit 0
elif [ "$count_malloc" = "1" ]; then
	grep -i "positive malloc" cinderella.static.pass1 | cut -d' ' -f4 > $TRUE_MALLOC
	echo "CINDERELLA: pass 1: detected true malloc"
	cat $TRUE_MALLOC
	echo "CINDERELLA TODO: rename detected malloc fn to cinderella::malloc"
	exit 0
fi

#
# Use dominator heuristic to find malloc
#    potential mallocs (dynamic): D = {A, B, C}
#    potential mallocs (static) : S = {X, Y, A, B, C}
#
#    F = D intersect S = {A, B, C}
#    call graph: A --> B --> C            ==>     A is malloc
#    call graph: A --> B --> C, A --> C   ==>     A is malloc
#
# Warning: static analyses must use the original variant id
#          as the clone id has all its functions pinned down so that zipr
#          doesn't move them. but pinning down functions will interfere
#          with the static analysis pass
#
echo "CINDERELLA PASS2: intersect dynamic and static analyses for malloc / turn on --dominator"
grep -i "positive malloc" cinderella.static.pass1 > $TMP
$SECURITY_TRANSFORMS_HOME/tools/cgclibc/cgclibc.exe $ORIG_VARIANT_ID --positive-inferences $TMP --negative-inferences cinderella.inferences.negative --dominator > cinderella.static.pass2
count_malloc=`grep "^static positive malloc" cinderella.static.pass2 | wc -l`
count_free=`grep "^static positive free" cinderella.static.pass2 | wc -l`

if [ "$count_malloc" = "1" ]; then
	grep -i "positive malloc" cinderella.static.pass2 | cut -d' ' -f4 > $TRUE_MALLOC
	echo "CINDERELLA: pass 2: detected true malloc"
	cat $TRUE_MALLOC
	echo "CINDERELLA TODO: rename detected malloc fn to cinderella::malloc"
	exit 0
fi

#
# Haven't yet found the true malloc/free
# Use simple clustering heuristic
#
if [ "$count_malloc" != "1" ] || [ "$count_free" != "1" ] ; then
	echo "CINDERELLA PASS3: with restrictions on malloc / turn on --dominator"
	grep -i "positive malloc" cinderella.static.pass2 > $TMP
	$SECURITY_TRANSFORMS_HOME/tools/cgclibc/cgclibc.exe $ORIG_VARIANT_ID --positive-inferences $TMP --negative-inferences cinderella.inferences.negative --dominator > cinderella.static.pass3
	count_malloc=`grep "^static positive malloc" cinderella.static.pass3 | wc -l`
	count_free=`grep "^static positive free" cinderella.static.pass3 | wc -l`
fi

echo "CINDERELLA: PASS3: #mallocs: $count_malloc  #frees: $count_free"

if [ "$count_malloc" = "1" ]; then
	grep -i "positive malloc" cinderella.static.pass3 | cut -d' ' -f4 > $TRUE_MALLOC
	echo "CINDERELLA: pass 3: detected true malloc"
	cat $TRUE_MALLOC
	echo "CINDERELLA TODO: rename detected malloc fn to cinderella::malloc"
	exit 0
fi

echo "CINDERELLA PASS3: with restrictions on malloc / turn on --dominator and --cluster"
grep -i "positive malloc" cinderella.static.pass3 > $TMP
$SECURITY_TRANSFORMS_HOME/tools/cgclibc/cgclibc.exe $ORIG_VARIANT_ID --positive-inferences $TMP --negative-inferences cinderella.inferences.negative --dominator --cluster > cinderella.static.pass4
count_malloc=`grep "^static positive malloc" cinderella.static.pass4 | wc -l`
count_free=`grep "^static positive free" cinderella.static.pass4 | wc -l`

echo "CINDERELLA: PASS4: #mallocs: $count_malloc  #frees: $count_free"
if [ "$count_malloc" = "1" ]; then
	grep -i "positive malloc" cinderella.static.pass4 | cut -d' ' -f4 > $TRUE_MALLOC
	echo "CINDERELLA: pass 4: detected true malloc"
	cat $TRUE_MALLOC
	echo "CINDERELLA TODO: rename detected malloc fn to cinderella::malloc"
	exit 0
fi

echo "CINDERELLA: TODO: handle realloc() and calloc()"
exit 0

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
		$PEASOUP_HOME/tools/do_prince.sh $cloneid `pwd`/$TESTABLE $PEASOUP_HOME/tools/cinderella.realloc.spec malloc.addresses $TRUE_MALLOC

		# @todo: fix this, not working at all
		echo "CINDERELLA SUCCESS: look for calloc"
		$PEASOUP_HOME/tools/do_prince.sh $cloneid `pwd`/$TESTABLE $PEASOUP_HOME/tools/cinderella.calloc.spec malloc.addresses $TRUE_MALLOC

echo "CINDERELLA TODO: if successful, rename detected calloc and realloc fns to cinderella::calloc, cinderella::realloc"
	fi
fi

exit 0
