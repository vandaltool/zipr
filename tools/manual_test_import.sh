#!/bin/sh

#
# Generate test script based on command and input/output description
#
# Assume we are in the top-level sub-directory created by ps_analyze.sh
#
# Usage: manual_test_import.sh [--name testname] [--infile in]* [--outfile out]*
# --name testname         'name of the input/output specification
# --infile in             'input file descriptions
# --outfile  out          'output file descriptions
# --prog cat              'the name of the program in the command
# --cmd "cat in > out"    'the command to invoke for the test
#

# extract from arguments

INFILES=""
OUTFILES=""
TEST_NAME=""
EXIT_CODE=""
while [ $# -gt 0 ]
do
  case "$1" in
	"--cmd")	CMD=$2; shift;;
	"--prog")	PROG=$2; shift;;
	"--infile")	INFILES="$2 $INFILES"; shift;;
	"--outfile")	OUTFILES="$2 $OUTFILES"; shift;;
	"--name")	TEST_NAME=$2; shift;;
	"--exitcode")	EXIT_CODE=$2; shift;;
 	*) break;;
  esac
  shift
done

TEST_TIMEOUT=60

echo "TEST_NAME = $TEST_NAME"
echo "INFILES = $INFILES"
echo "OUTFILES = $OUTFILES"
echo "EXIT_CODE = $EXIT_CODE"
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
if [ -z $TEST_NAME ]; then
  TEST_DIR=${PWD}/manual_tests/test.$$
  TEST_NAME=test.$$
else
  TEST_DIR=${PWD}/manual_tests/$TEST_NAME
fi
TEST_SPEC_DIR=${TEST_DIR}/spec
SPEC_INPUT_DIR=$TEST_SPEC_DIR/input
SPEC_OUTPUT_DIR=$TEST_SPEC_DIR/output
SPEC_EXIT_CODE_DIR=$TEST_SPEC_DIR/exitcode
EXIT_CODE_FILE=$SPEC_EXIT_CODE_DIR/exitcode.txt

TEST_ORIG_COVERAGE=$TEST_SPEC_DIR/coverage
TEST_ORIG_CMD_SCRIPT=$TEST_SPEC_DIR/generate_cover_orig_cmd.sh
TEST_DIR_XFORMED=$TEST_DIR/transformed
TEST_XFORMED_CMD_SCRIPT=$TEST_DIR_XFORMED/test_new_cmd.sh
TEST_XFORMED_OUTPUT_DIR=$TEST_DIR_XFORMED/output
TEST_XFORMED_EXIT_CODE_DIR=$TEST_DIR_XFORMED/exitcode
TEST_XFORMED_EXIT_CODE_FILE=$TEST_XFORMED_EXIT_CODE_DIR/exitcode.txt

mkdir -p $TEST_ORIG_COVERAGE
mkdir -p $SPEC_INPUT_DIR
mkdir -p $SPEC_OUTPUT_DIR
mkdir -p $SPEC_EXIT_CODE_DIR
mkdir -p $TEST_XFORMED_OUTPUT_DIR
mkdir -p $TEST_XFORMED_EXIT_CODE_DIR

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

if [ ! -z $EXIT_CODE ]; then
	echo $EXIT_CODE > $EXIT_CODE_FILE
fi

#---------------------------------------
# Original cmd/program
#---------------------------------------
#
# create script to run original command with coverage info
#

touch $TEST_ORIG_CMD_SCRIPT

echo "#!/bin/sh" >> $TEST_ORIG_CMD_SCRIPT

# cleanup input/output files
for i in $INFILES
do
  echo " rm $i 2>/dev/null" >> $TEST_ORIG_CMD_SCRIPT
done

for i in $OUTFILES
do
  echo " rm $i 2>/dev/null" >> $TEST_ORIG_CMD_SCRIPT
done

# stage in input (if any)
for i in $INFILES
do
  echo " cp $SPEC_INPUT_DIR/$i ." >> $TEST_ORIG_CMD_SCRIPT
done

echo "\$PEASOUP_HOME/tools/manual_cover.sh $TEST_ORIG_COVERAGE/executed_addresses.txt -- $CMD" >> $TEST_ORIG_CMD_SCRIPT

ln -s a.ncexe $TEST_SPEC_DIR/$PROG 

#---------------------------------------
# Transformed cmd/program
#---------------------------------------
#
# create script to run transformed command
#
touch $TEST_XFORMED_CMD_SCRIPT

echo "#!/bin/sh" >> $TEST_XFORMED_CMD_SCRIPT

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

# cleanup any old exit status code
echo " rm $TEST_XFORMED_EXIT_CODE_FILE 2>/dev/null" >> $TEST_XFORMED_CMD_SCRIPT

# stage in input (if any)
for i in $INFILES
do
  echo " cp $SPEC_INPUT_DIR/$i ." >> $TEST_XFORMED_CMD_SCRIPT
done

# run command (check for seg faults)
# @todo: we should really register the program exit code and check against it
echo "STRATA_SPRI_FILE=\$1 timeout $TEST_TIMEOUT setarch i386 -RL $CMD" >> $TEST_XFORMED_CMD_SCRIPT
echo "status=\$?" >> $TEST_XFORMED_CMD_SCRIPT
echo "echo \$status" >> $TEST_XFORMED_CMD_SCRIPT

if [ ! -z $EXIT_CODE ]; then
	echo "echo \$status > $TEST_XFORMED_EXIT_CODE_FILE" >> $TEST_XFORMED_CMD_SCRIPT
	echo "echo expected exit status: $EXIT_CODE" >> $TEST_XFORMED_CMD_SCRIPT
fi

echo "if [ \$status -eq 139 ]; then" >> $TEST_XFORMED_CMD_SCRIPT
echo "  exit \$status" >> $TEST_XFORMED_CMD_SCRIPT
echo "fi" >> $TEST_XFORMED_CMD_SCRIPT

# stage/move output files (if any)
for i in $OUTFILES
do
  echo " mv $i $TEST_XFORMED_OUTPUT_DIR" >> $TEST_XFORMED_CMD_SCRIPT
done

echo " exit \$status" >> $TEST_XFORMED_CMD_SCRIPT

chmod +x $TEST_XFORMED_CMD_SCRIPT

cp $PEASOUP_HOME/tools/run_stratafied.tmpl.sh $TEST_DIR_XFORMED/$PROG
chmod +x $TEST_DIR_XFORMED/$PROG
chmod +x $TEST_ORIG_CMD_SCRIPT

