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
COMMAND=ls

# test basic usage - does it crash?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe" --prog ls.exe --name ls.basic1 --exitcode 0

$COMMAND / >outputfile1 2>error.txt
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe /  > outputfile1 2>error.txt" --prog ls.exe --outfile outputfile1 --outfile error.txt --name ls.basic2 --exitcode 0

$COMMAND /  2>error.txt
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe /  2>error.txt" --prog ls.exe  --outfile error.txt --name ls.basic3 --exitcode 0

# test whole bunch of options -- output is non-deterministic so we just make sure we have the same number of lines
# also when something goes wrong during testing, the program usually just crashes
$COMMAND -ltarHksbBiXR inputfile1 | wc -l > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe -ltarHksbBiXR inputfile1 | wc -l > outputfile1" --prog ls.exe --infile inputfile1 --outfile outputfile1 --name ls.shload_flags --exitcode $exitcode

# test some option flags
$COMMAND -kfsZqp inputfile1 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe -kfsZqp inputfile1 > outputfile1" --prog ls.exe --infile inputfile1 --outfile outputfile1 --name ls.flag_combo --exitcode $exitcode

# test --ignore
$COMMAND --ignore=hello inputfile1 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe --ignore=hello inputfile1 > outputfile1" --prog ls.exe --infile inputfile1 --outfile outputfile1 --name ls.ignore_option --exitcode $exitcode

# test invalid options
$COMMAND -MX inputfile1 
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe -MX inputfile1" --prog ls.exe --infile inputfile1 --name ls.invalid_options --exitcode $exitcode

# test help 
$COMMAND --help | grep sort | grep -v ls | grep -v strata > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe --help | grep sort | grep -v ls | grep -v strata > outputfile1" --prog ls.exe --outfile outputfile1 --name ls.usage --exitcode $exitcode

# test recursive
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "mkdir /tmp/ls.tmp 2>/dev/null; ./ls.exe -R /tmp/ls.tmp" --prog ls.exe --name ls.recursive --exitcode 0

# test recursive 2
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "mkdir -p /tmp/ls.tmp.foobar 2>/dev/null; ./ls.exe -R /tmp/ls.tmp" --prog ls.exe --name ls.recursive2 --exitcode 0

# test basic usage - does it crash?
$COMMAND -t
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe -t" --prog ls.exe --name ls.timestamp --exitcode 0

# test basic usage - does it crash?
$COMMAND -u
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe -u" --prog ls.exe --name ls.u --exitcode 0

# test basic usage - does it crash?
$COMMAND -Z
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe -Z" --prog ls.exe --name ls.Z --exitcode 0

# test basic usage - does it crash?
$COMMAND --full-time
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./ls.exe --full-time" --prog ls.exe --name ls.fulltime --exitcode 0

# cleanup
rm inputfile1 outputfile1
