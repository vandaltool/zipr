#!/bin/sh

#
# Generate test script based on command and input/output description
#
# Assume we are in the top-level sub-directory created by ps_analyze.sh
#
# --cmd "cat"            'the command
# --args "i1 > o1"       'the rest of the command line
# --infile i1 ... in     'input file descriptions
# --outfile o1 ... on    'output file descriptions
# --stdout               'stash away stdout
# --stderr               'stash away stderr
#

# extract from arguments

INFILES=""
OUTFILES=""
while [ $# -gt 0 ]
do
  case "$1" in
	"--cmd")	CMD=$2; shift;;
	"--prog")	PROG=$2; shift;;
	"--infile")	INFILES="$2 $INFILES"; shift;;
	"--outfile")	OUTFILES="$2 $OUTFILES"; shift;;
 	*) break;;
  esac
  shift
done

echo "INFILES = $INFILES"
echo "OUTFILES = $OUTFILES"
echo "PROG = $PROG"
echo "CMD = $CMD"

#
# layout for manual_test_import
#
# ../test
# ../test/test.1/spec
# ../test/test.1/spec/input
# ../test/test.1/spec/output
# ../test/test.1/transformed
# ../test/test.1/transformed/input
# ../test/test.1/transformed/output

# setup test directory

PWD=`pwd`
TEST_DIR=${PWD}/test.$$
TEST_SPEC_DIR=${TEST_DIR}/spec
SPEC_INPUT_DIR=$TEST_SPEC_DIR/input
SPEC_OUTPUT_DIR=$TEST_SPEC_DIR/output
TEST_DIR_XFORMED=$TEST_DIR/transformed
TEST_XFORMED_CMD_SCRIPT=$TEST_DIR_XFORMED/test_new_cmd.sh
TEST_XFORMED_OUTPUT_DIR=$TEST_DIR_XFORMED/output

mkdir -p $SPEC_INPUT_DIR
mkdir -p $SPEC_OUTPUT_DIR
mkdir -p $TEST_XFORMED_OUTPUT_DIR

# copy input files
for i in $INFILES
do
  cp $i $SPEC_INPUT_DIR
done

# copy output files
for i in $OUTFILES
do
  cp $i $SPEC_OUTPUT_DIR
done

#---------------------------------------
# Transformed cmd/program
#---------------------------------------
#
# create script to run transformed command
#
touch $TEST_XFORMED_CMD_SCRIPT

# cleanup input/output files
for i in $INFILES
do
  echo " rm $i 2>/dev/null" >> $TEST_XFORMED_CMD_SCRIPT
done

for i in $OUTFILES
do
  echo " rm $i 2>/dev/null" >> $TEST_XFORMED_CMD_SCRIPT
  echo " rm $TEST_XFORMED_OUTPUT_DIR/$i 2>/dev/null" >> $TEST_XFORMED_CMD_SCRIPT
done

# stage in input 
for i in $INFILES
do
  echo " cp $SPEC_INPUT_DIR/$i ." >> $TEST_XFORMED_CMD_SCRIPT
done

# run command (check for seg faults)
# @todo: we should really register the program exit code and check against it
echo "STRATA_SPRI_FILE=\$1 $CMD" >> $TEST_XFORMED_CMD_SCRIPT
echo "if [ \$? -eq 139 ]; then" >> $TEST_XFORMED_CMD_SCRIPT
echo "  exit \$?" >> $TEST_XFORMED_CMD_SCRIPT
echo "fi" >> $TEST_XFORMED_CMD_SCRIPT

# stage/move output files
for i in $OUTFILES
do
  echo " mv $i $TEST_XFORMED_OUTPUT_DIR" >> $TEST_XFORMED_CMD_SCRIPT
done
chmod +x $TEST_XFORMED_CMD_SCRIPT

cp $PEASOUP_HOME/tools/run_stratafied.tmpl.sh $TEST_DIR_XFORMED/$PROG
chmod +x $TEST_DIR_XFORMED/$PROG

