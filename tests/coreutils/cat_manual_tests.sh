#!/bin/sh

#
# input/output specification for testing cat
#
echo "hello" > inputfile1

# test basic
cat inputfile1 > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar inputfile1 > outputfile1" --prog foobar  --infile inputfile1 --outfile outputfile1 --name basic

# test invalid options
cat -MX inputfile1 2>&1 | grep -vi cat > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -MX inputfile1 2>&1 | grep -vi stratafied | grep -vi cat > outputfile1" --prog foobar --infile inputfile1 --outfile outputfile1 --name invalid_options

# test help 
cat --help --version 2>&1 | grep -vi cat > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar --help --version 2>&1 | grep -vi stratafied | grep -vi cat > outputfile1" --prog foobar --outfile outputfile1 --name usage

# test lots of options
cat -Abetvsnu inputfile1 > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -Abetvsnu inputfile1 > outputfile1" --prog foobar  --infile inputfile1 --outfile outputfile1 --name shload_options

# cleanup
rm inputfile1 outputfile1
