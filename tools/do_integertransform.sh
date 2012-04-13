#!/bin/sh
#
# do_integertransform.sh <cloneId> <concolicDir> <timeout>
#
# pre: we are in the top-level directory created by ps_analyze.sh
#

# input
CLONE_ID=$1
CONCOLIC_DIR=$2
TIMEOUT=$3

# configuration variables
LIBC_FILTER=$PEASOUP_HOME/tools/libc_functions.txt   # libc and other system library functions
ANNOT_INFO=a.ncexe.infoannot                         # new annotation for integer checks

if [ -z $TIMEOUT ] ;
then
TIMEOUT=300                           # 5 mns
fi

TOP_DIR=`pwd`
INTEGER_ASPRI=a.irdb.integer.aspri
INTEGER_BSPRI=a.irdb.integer.bspri
INTEGER_WARNINGS_FILE=${TOP_DIR}/integer.warnings.addresses

touch $INTEGER_WARNINGS_FILE

echo "INT: transforming binary: cloneid=$CLONE_ID annotationInfoFile=$ANNOT_INFO"

if [ ! -f $ANNOT_INFO ]; then
	echo "INT: no info annotation file found -- skip integer transform"
	return 1
fi

# if C++ skip
#file a.ncexe | grep -i static | grep -i link | grep -i execut
#if [ $? -eq 0 ]; then
#  nm -a a.ncexe | grep __gnu_cxx
#  if [ $? -eq 0 ]; then
#     echo "INT: Statically-linked C++ program detected -- skipping"
#     exit 1
#  fi
#fi

# Sanity check -- did Grace produce anything?
#
# If so, we run the integer transform on Grace-generated inputs on a cloned copy
# Any instructions that trigger a C1/Integer detector is flagged as a warning (instead of an error)
# This is designed to prevent false positives
#
# Comment out this block of code if you don't want to even attempt to detect false positives

#. $PEASOUP_HOME/tools/grace_utils.sh || echo "INT: could not locate grace utility scripts"
#get_grace_number_inputs_executed $CONCOLIC_DIR
#if [ ! $? -eq 0 ]; then
#	echo "INT: Grace executed at least 1 input"

#	echo "INT: Clone program"
#	$SECURITY_TRANSFORMS_HOME/libIRDB/test/clone.exe $CLONE_ID clone.id
#	tempcloneid=`cat clone.id`
#
	# Pass 1
	#    - Transform program and run against all Grace-generated inputs using a policy of continued execution when an integer detector triggers (we want to catch all detection messages)
	#    - Keep track of all inputs that trigger a C1 diagnostic and put in a list
#	echo "INT: Integer transform on cloned copy"
#	$SECURITY_TRANSFORMS_HOME/tools/transforms/integertransformdriver.exe $tempcloneid $ANNOT_INFO $LIBC_FILTER

    # generate aspri, and assemble it to bspri
#	echo "INT: Generate temporary aspri --> bspri for integer transform"
#	$SECURITY_TRANSFORMS_HOME/libIRDB/test/generate_spri.exe $tempcloneid $INTEGER_ASPRI 
#	$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm $INTEGER_ASPRI $INTEGER_BSPRI stratafier.o.exe

#	if [ $? -eq 0 ]; then
		# produce list of instruction addresses that trigger an integer detector
#		echo "INT: false positives detection activated"
#		timeout $TIMEOUT $PEASOUP_HOME/tools/integer_replay.sh $TOP_DIR/a.stratafied $CONCOLIC_DIR $TOP_DIR/$INTEGER_BSPRI $INTEGER_WARNINGS_FILE
#		sort $INTEGER_WARNINGS_FILE | uniq > $INTEGER_WARNINGS_FILE.$$
#		mv $INTEGER_WARNINGS_FILE.$$ $INTEGER_WARNINGS_FILE
#
#		cd $TOP_DIR   # restore working dir (just in case)
#	else
#		echo "Error generating integer transforms -- skip replay step to detect benign false positives"
#	fi
#fi

# restore working dir (just in case)
cd $TOP_DIR   

# Transform program but for each instruction present in the list above, use a "CONTINUE" policy to emit a warning (instead of the default CONTROLLED EXIT policy)
echo "INT: Final integer transform"
$SECURITY_TRANSFORMS_HOME/tools/transforms/integertransformdriver.exe $CLONE_ID $ANNOT_INFO $LIBC_FILTER $INTEGER_WARNINGS_FILE --saturating-arithmetic

#$SECURITY_TRANSFORMS_HOME/tools/transforms/integertransformdriver.exe $CLONE_ID $ANNOT_INFO $LIBC_FILTER $INTEGER_WARNINGS_FILE 
