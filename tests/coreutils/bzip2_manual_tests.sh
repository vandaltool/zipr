#!/bin/sh

echo "hello" > inputfile1

# test functionality
bzip2 inputfile1 -kt -v -v -2 inputfile1 -c > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -kt -v -v -2 inputfile1 -c > outputfile1" --prog foobar --infile inputfile1 --outfile outputfile1

# test help 
bzip2 --help 2>&1| grep -vi bzip2 | grep -vi stratafied > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar --help 2>&1 | grep -vi bzip2 | grep -vi stratafied > outputfile1" --prog foobar --outfile outputfile1 --name usage

# cleanup
rm inputfile1 outputfile1
