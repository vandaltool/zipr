#!/bin/sh

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name NoInput --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 999999 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 999999 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name BadYear1 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 0 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 0 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name BadYear2 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 0 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 0 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name BadMonth1 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 13 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 13 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name BadMonth2 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 13 0 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 13 0 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name BadYearMonth1 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 0 999999 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 0 999999 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name BadYearMonth2 --exitcode $exitcode


$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Year2001 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 1500 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 1500 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Year1500 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 1 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 1 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Year1 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 2000 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 2000 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name LeapYear2000 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 1804 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 1804 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name LeapYear1804 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 1 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 1 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Jan2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 2 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 2 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Feb2011 --exitcode $exitcode


$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 3 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 3 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name March2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 4 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 4 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name April2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 5 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 5 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name May2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 6 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 6 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name June2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 7 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 7 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name July2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 8 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 8 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name August2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 9 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 9 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Sept2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 10 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 10 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Oct2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 11 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 11 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Nov2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 12 2011 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 12 2011 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name Dec2011 --exitcode $exitcode

$SECURITY_TRANSFORMS_HOME/tests/simple/cal_nop.exe 2 1952 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cal_nop.exe 2 1952 > outputfile1" --prog cal_nop.exe  --outfile outputfile1 --name LeapYearFeb1952 --exitcode $exitcode


rm outputfile1

