#!/bin/sh

#
# input/output specification for testing ls
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
# NOTE: put the most complicated tests (those that are most likely to fail) first!

echo "hello" > inputfile1

# test whole bunch of options -- output is non-deterministic so we just make sure we have the same number of lines
# also when something goes wrong during testing, the program usually just crashes
ls -ltarHksbBiXR inputfile1 | wc -l > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -ltarHksbBiXR inputfile1 | wc -l > outputfile1" --prog ls --infile inputfile1 --outfile outputfile1 --name ls.shload_flags

# test some option flags
ls -kfsZqp inputfile1 > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -kfsZqp inputfile1 > outputfile1" --prog ls --infile inputfile1 --outfile outputfile1 --name ls.flag_combo

# test --ignore
ls --ignore=hello inputfile1 > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls --ignore=hello inputfile1 > outputfile1" --prog ls --infile inputfile1 --outfile outputfile1 --name ls.ignore_option
# test invalid options
ls -MX inputfile1 | grep -vi invalid | grep -vi usage > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -MX inputfile1 | grep -vi invalid | grep -vi usage > outputfile1" --prog ls --infile inputfile1 --outfile outputfile1 --name ls.invalid_options

# test help 
ls --help | grep -vi report | grep -vi usage > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls --help | grep -vi report | grep -vi usage > outputfile1" --prog ls --outfile outputfile1 --name ls.usage

# cleanup
rm inputfile1 outputfile1
