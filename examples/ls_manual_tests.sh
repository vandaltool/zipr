#!/bin/sh

# input/output specification for testing
#
# assumptions:
#      - deterministic tests
#      - will be run from the top-level subdirectory created by ps_analyze.sh
#
# gotchas:
#      - when ls reports an error, it uses argv[0]. This causes problems as we rename the program name
#        we filter out the lines that use argv[0] as a workaround
#      - timestamp info will differ b/c we're copying files around as part of manual_test_import
#

echo "hello" > i1

# basic functionality -- don't bother with comparing outputs
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar" --prog foobar 

# test invalid options
ls -MX i1 | grep -vi invalid | grep -vi usage > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -MX i1 | grep -vi invalid | grep -vi usage > o1" --prog foobar --infile i1 --outfile o1

# test help 
ls --help | grep -vi report | grep -vi usage > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar --help | grep -vi report | grep -vi usage > o1" --prog foobar --outfile o1

# test some option flags
ls -kfsZqp i1 > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -kfsZqp i1 > o1" --prog foobar --infile i1 --outfile o1

# test --ignore
ls --ignore=hello i1 > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar --ignore=hello i1 > o1" --prog foobar --infile i1 --outfile o1

# test whole bunch of options -- output is non-deterministic so we just make sure we have the same number of lines
ls -ltarHksbBiXR i1 | wc -l > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -ltarHksbBiXR i1 | wc -l > o1" --prog foobar --infile i1 --outfile o1

# cleanup
rm i1 o1

exit 0

#
# enough testing for now
#

ls -aw i1 > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -aw i1 > o1" --prog foobar --infile i1 --outfile o1

ls -R . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -R . > o1" --prog foobar --outfile o1

ls -hBG . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -hBG . > o1" --prog foobar --outfile o1

ls -m . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -m . > o1" --prog foobar --outfile o1

ls -X . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -X . > o1" --prog foobar --outfile o1

ls -x . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -x . > o1" --prog foobar --outfile o1
