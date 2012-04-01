#!/bin/sh

#
# input/output specification for testing cat
#
echo "hello" > inputfile1

cat inputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat inputfile1" --prog cat  --infile inputfile1 --name basic1 --exitcode $exitcode


# test lots of options
cat -Abetvsnu inputfile1 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat -Abetvsnu inputfile1 > outputfile1" --prog cat  --infile inputfile1 --outfile outputfile1 --name shload_options --exitcode $exitcode

# test basic
cat inputfile1 > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat inputfile1 > outputfile1" --prog cat  --infile inputfile1 --outfile outputfile1 --name basic --exitcode $exitcode

# test invalid options
cat -MX inputfile1 2>&1 | grep -vi cat > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat -MX inputfile1 2>&1 | grep -vi stratafied | grep -vi cat > outputfile1" --prog cat --infile inputfile1 --outfile outputfile1 --name invalid_options --exitcode $exitcode

# test help 
cat --help --version 2>&1 | grep -vi cat > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat --help --version 2>&1 | grep -vi stratafied | grep -vi cat > outputfile1" --prog cat --outfile outputfile1 --name usage --exitcode $exitcode

cat --version 2>&1 | grep -vi cat > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./cat --version 2>&1 | grep -vi stratafied | grep -vi cat > outputfile1" --prog cat --outfile outputfile1 --name version --exitcode $exitcode



# cleanup
rm inputfile1 outputfile1
