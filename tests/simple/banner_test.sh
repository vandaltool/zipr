#!/bin/sh

$SECURITY_TRANSFORMS_HOME/tests/simple/banner_nop.exe hello > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./banner.exe hello > outputfile1" --prog banner.exe  --outfile outputfile1 --name hello

$SECURITY_TRANSFORMS_HOME/tests/simple/banner_nop.exe 12345 > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./banner.exe 12345 > outputfile1" --prog banner.exe  --outfile outputfile1 --name numbers

$SECURITY_TRANSFORMS_HOME/tests/simple/banner_nop.exe Hello World 1234 > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./banner.exe Hello World 1234 > outputfile1" --prog banner.exe  --outfile outputfile1 --name HelloWorld1234