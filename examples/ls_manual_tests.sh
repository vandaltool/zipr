#!/bin/sh

mkdir tmp.$$
cd tmp.$$
mkdir subdir
echo "hello" > subdir/hello
cp /etc/passwd passwd

# test 1
ls > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar > o1" --prog foobar --outfile o1

# test 2
ls -R . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -R . > o1" --prog foobar --outfile o1

# test 3
ls i1 > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar i1 > o1" --prog foobar --infile i1 --outfile o1

# test 4
ls -aw . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -aw . > o1" --prog foobar --outfile o1

# test 5
ls -chBG . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -chBG . > o1" --prog foobar --outfile o1

# test 6
ls -s . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -s . > o1" --prog foobar --outfile o1

# test 7
ls -m . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -m . > o1" --prog foobar --outfile o1

# test 8: invalid option
ls -MX . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -MX . > o1" --prog foobar --outfile o1

# test 9
ls --help > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar --help > o1" --prog foobar --outfile o1

# test 10
ls -Zlt . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -Zlt . > o1" --prog foobar --outfile o1

# test 11
ls -X . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -X . > o1" --prog foobar --outfile o1

# test 12
ls -x . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -x . > o1" --prog foobar --outfile o1

# test 13
ls -kif . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar -kif . > o1" --prog foobar --outfile o1

# test 14
ls --ignore=hello . > o1
$PEASOUP_HOME/tools/manual_test_import.sh --cmd "./foobar --ignore=hello . > o1" --prog foobar --outfile o1
