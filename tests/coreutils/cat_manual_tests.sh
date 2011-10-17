#!/bin/sh

#
# input/output specification for testing cat
#
echo "hello" > inputfile1

# test lots of options
cat -Abetvsnu inputfile1 > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat -Abetvsnu inputfile1 > outputfile1" --prog cat  --infile inputfile1 --outfile outputfile1 --name cat.shload_options

# test basic
cat inputfile1 > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat inputfile1 > outputfile1" --prog cat  --infile inputfile1 --outfile outputfile1 --name cat.basic

# test invalid options
cat -MX inputfile1 2>&1 | grep -vi cat > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat -MX inputfile1 2>&1 | grep -vi stratafied | grep -vi cat > outputfile1" --prog cat --infile inputfile1 --outfile outputfile1 --name cat.invalid_options

# test help 
cat --help --version 2>&1 | grep -vi cat > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat --help --version 2>&1 | grep -vi stratafied | grep -vi cat > outputfile1" --prog cat --outfile outputfile1 --name cat.usage

# cleanup
rm inputfile1 outputfile1
