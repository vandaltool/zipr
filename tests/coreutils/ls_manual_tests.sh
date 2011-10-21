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

# test basic usage - does it crash?
ls
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls" --prog ls --name ls.basic --exitcode 0

# test whole bunch of options -- output is non-deterministic so we just make sure we have the same number of lines
# also when something goes wrong during testing, the program usually just crashes
ls -ltarHksbBiXR inputfile1 | wc -l > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -ltarHksbBiXR inputfile1 | wc -l > outputfile1" --prog ls --infile inputfile1 --outfile outputfile1 --name ls.shload_flags --exitcode $exitcode

# test some option flags
ls -kfsZqp inputfile1 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -kfsZqp inputfile1 > outputfile1" --prog ls --infile inputfile1 --outfile outputfile1 --name ls.flag_combo --exitcode $exitcode

# test --ignore
ls --ignore=hello inputfile1 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls --ignore=hello inputfile1 > outputfile1" --prog ls --infile inputfile1 --outfile outputfile1 --name ls.ignore_option --exitcode $exitcode

# test invalid options
ls -MX inputfile1 
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -MX inputfile1" --prog ls --infile inputfile1 --name ls.invalid_options --exitcode $exitcode

# test help 
ls --help | grep sort | grep -v ls | grep -v strata > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls --help | grep sort | grep -v ls | grep -v strata > outputfile1" --prog ls --outfile outputfile1 --name ls.usage --exitcode $exitcode

# test recursive
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "mkdir /tmp/ls.tmp 2>/dev/null; ./ls -R /tmp/ls.tmp" --prog ls --name ls.recursive --exitcode 0

# test recursive 2
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "mkdir -p /tmp/ls.tmp.foobar 2>/dev/null; ./ls -R /tmp/ls.tmp" --prog ls --name ls.recursive2 --exitcode 0

# test basic usage - does it crash?
ls -t
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -t" --prog ls --name ls.timestamp --exitcode 0

# test basic usage - does it crash?
ls -u
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -u" --prog ls --name ls.u --exitcode 0

# test basic usage - does it crash?
ls -Z
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls -Z" --prog ls --name ls.Z --exitcode 0

# test basic usage - does it crash?
ls --full-time
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls --full-time" --prog ls --name ls.fulltime --exitcode 0

# cleanup
rm inputfile1 outputfile1
