#!/bin/bash

TMP_ORIG=/tmp/testelfdep.ls.orig.$$
TMP_ORIG2=/tmp/testelfdep.ls.orig.2.$$
TMP_ELFDEP=/tmp/testelfdep.ls.elfdep.$$

cleanup_files()
{
	rm /tmp/testelfdep.ls* >/dev/null 2>&1
}

cleanup()
{
	echo "************"
	echo "test failed."
	echo "************"

	cleanup_files
	exit 1
}

	
# make sure xforms are built
scons || cleanup

$PSZ /bin/ls ./xxx -c move_globals=on -o move_globals:--elftables -c edt=on || cleanup
 
/bin/ls /tmp > $TMP_ORIG || cleanup

./xxx /tmp > $TMP_ELFDEP || cleanup

echo "Verify external vars was overwritten"
grep "var = 0" $TMP_ELFDEP || cleanup
grep "var = 1" $TMP_ELFDEP || cleanup

echo "Verify same output"
grep -v "var =" $TMP_ELFDEP > $TMP_ORIG2
diff $TMP_ORIG2 $TMP_ELFDEP

cleanup_files

echo
echo "test passed."
echo
