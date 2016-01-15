#orig_progs="bzip2 du egrep fgrep grep ls objdump readelf sort tar tcpdump touch"
orig_progs="cal"

if [ ! -d tmp_test_area ]; then
	mkdir tmp_test_area
fi

pushd .
cd tmp_test_area

progs_pass=""
progs_fail=""

for prog in $orig_progs
do
	# use same name 
	protected=${prog}
	temp_dir=tmp.${prog}.protected

	if [ -d $temp_dir ]; then
		rm -fr $temp_dir
	fi

	if [ -e $protected ]; then
		rm $protected
	fi

	echo "TEST ${prog}: Protecting..."

	#
	# change options here for different kinds of protections
	#
	FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh `which $prog` $protected --backend zipr --step selective_cfi=on --step-option selective_cfi:--color --step-option selective_cfi:--no-protect-jumps --tempdir $temp_dir > test_${prog}.ps.log 2>&1
	if [ ! $? -eq 0 ]; then
		echo "TEST ${prog}: FAILED to peasoupify"
		progs_fail="$progs_fail $prog"
		continue
	fi

	echo "TEST ${prog}: Running tests..."
	../$prog/test_script.sh `which $prog` ./$protected > test_${prog}.log 2>&1
	if [ $? -eq 0 ]; then
		echo "TEST ${prog}: PASS"
		progs_pass="$progs_pass $prog"
	else
		echo "TEST ${prog}: FAIL"
		progs_fail="$progs_fail $prog"
	fi
done

echo "PASS: $progs_pass"
echo "FAIL: $progs_fail"

popd
