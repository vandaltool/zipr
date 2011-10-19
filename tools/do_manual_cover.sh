#!/bin/sh

#
# Generate coverage information for each manual test
# Aggregate all coverage into one final file
#

PEASOUP_DIR=`pwd`
MANUAL_TEST_DIR=$PEASOUP_DIR/manual_tests
COVER_SCRIPT=generate_cover_orig_cmd.sh
AGGREGATE_COVERAGE=$PEASOUP_DIR/manual_coverage.txt  # final output file

ls $MANUAL_TEST_DIR/* >/dev/null 2>/dev/null
if [ ! $? -eq 0 ]; then
  echo "do_manual_cover.sh: error: no manual test specifications found -- exiting"
  exit 1
fi

echo "Inside do_manual_cover.sh"

touch $AGGREGATE_COVERAGE

for testname in `ls $MANUAL_TEST_DIR`
do
  echo "do_manual_cover.sh: $MANUAL_TEST_DIR/$testname/spec/$COVER_SCRIPT"

  cd $MANUAL_TEST_DIR/$testname/spec/
  $MANUAL_TEST_DIR/$testname/spec/$COVER_SCRIPT
  cd -

  cat $MANUAL_TEST_DIR/$testname/spec/coverage/executed_addresses.txt >> $AGGREGATE_COVERAGE
done

sort $AGGREGATE_COVERAGE | uniq > tmp.$$
mv tmp.$$ $AGGREGATE_COVERAGE

exit 0
