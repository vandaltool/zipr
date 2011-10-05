#!/bin/sh

echo "hello" > inputfile1
cp `which ls` > ls.orig
bzip2 -k -c ls.orig > ls.orig.bz2

# test compression level 1
bzip2 inputfile1 -k -1 -c > outputfile1 2> err
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./BZIP2 -k -1 inputfile1 -c > outputfile1 2> err" --prog BZIP2 --infile inputfile1 --outfile outputfile1 --outfile err --name bzip2.k1

# test compression level 5
bzip2 inputfile1 -k -5 -c > outputfile1 2> err
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./BZIP2 -k -5 inputfile1 -c > outputfile1 2> err" --prog BZIP2 --infile inputfile1 --outfile outputfile1 --outfile err --name bzip2.k5

# test compression level 9
bzip2 inputfile1 -k -9 --stdout > outputfile1 2> err
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./BZIP2 -k -9 inputfile1 --stdout > outputfile1 2> err" --prog BZIP2 --infile inputfile1 --outfile outputfile1 --outfile err --name bzip2.k9

# test on larger file (like ls)
bzip2 -k -8 -c ls.orig > ls.orig.bz2 2> err
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./BZIP2 -k -8 ls.orig 2> err" --prog BZIP2 --infile ls.orig --outfile err --outfile ls.orig.bz2 --name bzip2.ls

# test small memory footprint option
bzip2 -k -s -c ls.orig > ls.orig.bz2 2> err
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./BZIP2 -k -s ls.orig 2> err" --prog BZIP2 --infile ls.orig --outfile err --outfile ls.orig.bz2 --name bzip2.ls

# test integrity check
bzip2 -t ls.orig.bz2 
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./BZIP2 -t ls.orig.bz2" --prog BZIP2 --name bzip2.integrity --infile ls.orig.bz2

# test help 
bzip2 --help 2>&1| grep -vi bzip2 | grep -vi stratafied > outputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./BZIP2 --help 2>&1 | grep -vi bzip2 | grep -vi stratafied > outputfile1" --prog BZIP2 --outfile outputfile1 --name usage

# cleanup
rm inputfile1 outputfile1 err ls.orig ls.orig.bz2
