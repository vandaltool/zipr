#!/bin/sh
cp $SPEC_HOME/benchspec/CPU2006/401.bzip2/data/all/input/input.combined specinputfile1
COMMAND=$SECURITY_TRANSFORMS_HOME/tests/coreutils/bzip2_nop.exe

echo "hello" > inputfile1
cp `which ls` ls.orig

$COMMAND inputfile1 2>outputfile1
exitcode=$?
echo "hello" > inputfile1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 inputfile1 2>outputfile1" --prog bzip2 --infile inputfile1 --outfile outputfile1 --outfile inputfile1.bz2 --name bzip2.basic --exitcode $exitcode --cleanup inputfile1.bz2

cp $SECURITY_TRANSFORMS_HOME/tests/coreutils/bzip2-1.0.6/sample1.ref sample1.ref
cp $SECURITY_TRANSFORMS_HOME/tests/coreutils/bzip2-1.0.6/sample2.ref sample2.ref
cp $SECURITY_TRANSFORMS_HOME/tests/coreutils/bzip2-1.0.6/sample3.ref sample3.ref
cp $SECURITY_TRANSFORMS_HOME/tests/coreutils/bzip2-1.0.6/sample1.bz2 sample1.bz2
cp $SECURITY_TRANSFORMS_HOME/tests/coreutils/bzip2-1.0.6/sample2.bz2 sample2.bz2
cp $SECURITY_TRANSFORMS_HOME/tests/coreutils/bzip2-1.0.6/sample3.bz2 sample3.bz2

$COMMAND -1 < sample1.ref > sample1.rb2 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -1 < sample1.ref > sample1.rb2 2>error.txt" --prog bzip2 --infile sample1.ref --outfile sample1.rb2 --outfile error.txt --name bzip2.stock1 --exitcode $exitcode

$COMMAND -2 < sample2.ref > sample2.rb2 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -2 < sample2.ref > sample2.rb2 2>error.txt" --prog bzip2 --infile sample2.ref --outfile sample2.rb2 --outfile error.txt --name bzip2.stock2 --exitcode $exitcode

$COMMAND -3 < sample3.ref > sample3.rb2 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -3 < sample3.ref > sample3.rb2 2>error.txt" --prog bzip2 --infile sample3.ref --outfile sample3.rb2 --outfile error.txt --name bzip2.stock3 --exitcode $exitcode

$COMMAND -d < sample1.bz2 > sample1.tst 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -d < sample1.bz2 > sample1.tst 2>error.txt" --prog bzip2 --infile sample1.bz2 --outfile sample1.tst --outfile error.txt --name bzip2.stock4 --exitcode $exitcode

$COMMAND -d < sample2.bz2 > sample2.tst 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -d < sample2.bz2 > sample2.tst 2>error.txt" --prog bzip2 --infile sample2.bz2 --outfile sample2.tst --outfile error.txt --name bzip2.stock5 --exitcode $exitcode

$COMMAND -ds < sample3.bz2 > sample3.tst 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -ds < sample3.bz2 > sample3.tst 2>error.txt" --prog bzip2 --infile sample3.bz2 --outfile sample3.tst --outfile error.txt --name bzip2.stock6 --exitcode $exitcode

cp sample1.ref sample1.ref.bak

$COMMAND sample1.ref 2>error.txt
exitcode=$?
cp sample1.ref.bak sample1.ref
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2  sample1.ref 2>error.txt" --prog bzip2 --infile sample1.ref --outfile sample1.ref.bz2 --outfile error.txt --name bzip2.stock7 --exitcode $exitcode --cleanup sample1.ref.bz2

rm sample1.ref.bz2

$COMMAND -1 sample1.ref 2>error.txt
exitcode=$?
cp sample1.ref.bak sample1.ref
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -1 sample1.ref 2>error.txt" --prog bzip2 --infile sample1.ref --outfile sample1.ref.bz2 --outfile error.txt --name bzip2.stock8 --exitcode $exitcode --cleanup sample1.ref.bz2

cp sample2.ref sample2.ref.bak

$COMMAND -2 sample2.ref 2>error.txt
exitcode=$?
cp sample2.ref.bak sample2.ref
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -2 sample2.ref 2>error.txt" --prog bzip2 --infile sample2.ref --outfile sample2.ref.bz2 --outfile error.txt --name bzip2.stock9 --exitcode $exitcode --cleanup sample2.ref.bz2

cp sample3.ref sample3.ref.bak

$COMMAND -3 sample3.ref  2>error.txt
exitcode=$?
cp sample3.ref.bak sample3.ref
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -3 sample3.ref 2>error.txt" --prog bzip2 --infile sample3.ref --outfile sample3.ref.bz2 --outfile error.txt --name bzip2.stock10 --exitcode $exitcode --cleanup sample3.ref.bz2

cp sample1.bz2 sample1.zipbak
$COMMAND -d sample1.bz2  2>error.txt
exitcode=$?
cp sample1.zipbak sample1.bz2
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -d sample1.bz2 2>error.txt" --prog bzip2 --infile sample1.bz2 --outfile sample1 --outfile error.txt --name bzip2.stock11 --exitcode $exitcode --cleanup sample1

cp sample2.bz2 sample2.zipbak
$COMMAND -d sample2.bz2  2>error.txt
exitcode=$?
cp sample2.zipbak sample2.bz2
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -d sample2.bz2 2>error.txt" --prog bzip2 --infile sample2.bz2 --outfile sample2 --outfile error.txt --name bzip2.stock12 --exitcode $exitcode --cleanup sample2

cp sample3.bz2 sample3.zipbak
$COMMAND -ds sample3.bz2 2>error.txt
exitcode=$?
cp sample3.zipbak sample3.bz2
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -ds sample3.bz2 2>error.txt" --prog bzip2 --infile sample3.bz2 --outfile sample3 --outfile error.txt --name bzip2.stock13 --exitcode $exitcode --cleanup sample3


echo "hello" > inputfile1
# test small memory footprint option
$COMMAND -k -s -c ls.orig > ls.orig.bz2 2>error.txt 
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -k -s -c ls.orig > ls.orig.bz2 2>error.txt" --prog bzip2 --infile ls.orig --outfile ls.orig.bz2 --outfile error.txt --name bzip2.footprint --exitcode $exitcode

# test compression level 1
$COMMAND inputfile1 -k -1 -c > outputfile1 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -k -1 inputfile1 -c > outputfile1 2>error.txt" --prog bzip2 --infile inputfile1 --outfile outputfile1 --outfile error.txt --outfile err --name bzip2.k1 --exitcode $exitcode

# test compression level 5
$COMMAND inputfile1 -k -5 -c > outputfile1 2> error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -k -5 inputfile1 -c > outputfile1 2> error.txt" --prog bzip2 --infile inputfile1 --outfile outputfile1 --outfile error.txt --name bzip2.k5 --exitcode $exitcode

# test compression level 9
$COMMAND inputfile1 -k -9 --stdout > outputfile1 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -k -9 inputfile1 --stdout > outputfile1 2>error.txt" --prog bzip2 --infile inputfile1 --outfile outputfile1 --outfile error.txt --name bzip2.k9 --exitcode $exitcode

# test on larger file (like ls)
$COMMAND -k -8 -c ls.orig > ls.orig.bz2 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -k -8 -c ls.orig > ls.orig.bz2 2>error.txt" --prog bzip2 --infile ls.orig --outfile ls.orig.bz2 --outfile error.txt --name bzip2.ls --exitcode $exitcode

# try a large spec benchmark
#bzip2 -k -8 -c specinputfile1 > spec.input.combined.bz2 
#exitcode=$?
#$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -k -8 -c specinputfile1 > spec.input.combined.bz2" --prog bzip2 --infile specinputfile1 --outfile spec.input.combined.bz2  --name bzip2.spec --exitcode $exitcode

# test integrity check
$COMMAND -t ls.orig.bz2 2>error.txt
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 -t ls.orig.bz2 2>error.txt" --prog bzip2 --name bzip2.integrity --outfile error.txt --infile ls.orig.bz2 --name bzip2.integrity --exitcode $exitcode

# test help 
$COMMAND --help 2>&1| grep -vi bzip2 | grep -vi stratafied > outputfile1
exitcode=$?
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./bzip2 --help 2>&1 | grep -vi bzip2 | grep -vi stratafied > outputfile1" --prog bzip2 --outfile outputfile1 --name bzip2.usage --exitcode $exitcode


# cleanup
rm inputfile1 specinputfile1 outputfile1 error.txt ls.orig ls.orig.bz2 inputfile1.bz2 sample1 sample2 sample3 sample1.bz2 sample2.bz2 sample3.bz2 sample1.ref sample2.ref sample3.ref sample1.tst sample2.tst sample3.tst sample1.rb2 sample2.rb2 sample3.rb2  sample1.ref.bz2 sample2.ref.bz2 sample3.ref.bz2 sample1.ref.bak sample2.ref.bak sample3.ref.bak sample1.zipbak sample2.zipbak sample3.zipbak