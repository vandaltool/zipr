#!/bin/sh

transformed_bzip2=$1
echo "WRAPPER: command: $transformed_bzip2"

TMP1=tmp.1.$$
TMP2=tmp.2.$$
TMP3=tmp.3.$$

echo "foobar" > $TMP1
bzip2 $TMP1
mv $TMP1.bz2 $TMP2

echo "WRAPPER: issuing: $1 $TMP1"
echo "foobar" > $TMP1
$transformed_bzip2 $TMP1
mv $TMP1.bz2 $TMP3

echo "WRAPPER: diffing"
diff $TMP2 $TMP3
status=$?

echo "WRAPPER: cleanup"
rm -f $TMP1 $TMP2 $TMP3
exit $status
