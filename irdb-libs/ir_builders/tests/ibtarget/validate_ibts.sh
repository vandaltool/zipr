# build original and protected test programs
make clean all protected
if [ ! $? -eq 0 ]; then
	echo "failed to compile test programs"
	exit 1
fi

export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:."

ALL_PASS_STATUS=0
# compare output
for i in `ls *.exe`
do
	orig=$i
	base=$(basename $orig .exe)
	protected=${base}.protected

	echo $orig $base $protected
	./$orig 1 2 > $$.tmp.orig
	./$protected 1 2 > $$.tmp.protected
	diff $$.tmp.orig $$.tmp.protected
	if [ ! $? -eq 0 ]; then
		echo "FAIL: Protected program ${base}.protected does not match original program"
		ALL_PASS_STATUS=1
	else
		echo "PASS: $orig vs. $protected"
	fi

	rm $$.tmp.orig $$.tmp.protected
done

echo
if [ $ALL_PASS_STATUS -eq 0 ]; then
	echo "PASS: all tests passed"
else
	echo "FAIL: one or more test failed"
fi

