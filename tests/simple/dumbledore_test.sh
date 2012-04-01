#!/bin/sh

$SECURITY_TRANSFORMS_HOME/tests/simple/dumbledore_cmd_nop.exe > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./dumbledore_cmd.exe > outputfile1" --prog dumbledore_cmd.exe  --outfile outputfile1 --name NoInput

$SECURITY_TRANSFORMS_HOME/tests/simple/dumbledore_cmd_nop.exe Ben > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./dumbledore_cmd.exe Ben > outputfile1" --prog dumbledore_cmd.exe  --outfile outputfile1 --name DInput

$SECURITY_TRANSFORMS_HOME/tests/simple/dumbledore_cmd_nop.exe "Wizard in Train" > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./dumbledore_cmd.exe \"Wizard in Train\" > outputfile1" --prog dumbledore_cmd.exe  --outfile outputfile1 --name DInput2

$SECURITY_TRANSFORMS_HOME/tests/simple/dumbledore_cmd_nop.exe "Wizard in Training" > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./dumbledore_cmd.exe \"Wizard in Training\" > outputfile1" --prog dumbledore_cmd.exe  --outfile outputfile1 --name BInput

# cleanup
rm outputfile1
