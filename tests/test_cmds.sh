# specify configs here
configs="zipr scfi scfi.color scfi.color.nojmps kill_deads"
# specify programs to test
orig_progs="bzip2 cal du grep ls objdump readelf sort tar tcpdump touch"
build_only=0

for config in $configs
do

echo "----------------------------------"
echo "TEST CONFIGURATION: $config"
progs_pass=""
progs_fail=""
progs_fail_peasoup=""

if [ -d tmp_test_area.$config ]; then
	rm -fr tmp_test_area.$config
fi

mkdir tmp_test_area.$config

pushd .
cd tmp_test_area.$config

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

	echo "TEST ($config) ${prog}: Protecting..."

	case $config in
		zipr)
			$PEASOUP_HOME/tools/ps_analyze.sh `which $prog` $protected --backend zipr --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi.color)
			FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh `which $prog` $protected --backend zipr --step selective_cfi=on --step-option selective_cfi:--color --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi.color.nojmps)
			FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh `which $prog` $protected --backend zipr --step selective_cfi=on --step-option selective_cfi:--color --step-option selective_cfi:--no-protect-jumps --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi)
			FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh `which $prog` $protected --backend zipr --step selective_cfi=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		kill_deads)
			$PEASOUP_HOME/tools/ps_analyze.sh `which $prog` $protected --backend zipr --step kill_deads=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		*)
			echo "Unknown configuration requested"
			continue
		;;
	esac

	if [ ! $? -eq 0 ]; then
		echo "TEST ($config) ${prog}: FAILED to peasoupify"
		progs_fail_peasoup="$progs_fail_peasoup $prog"
	fi

	if [ $build_only -eq 1 ]; then
		continue
	fi

	echo "TEST $config ${prog}: Running tests..."
	timeout 300 ../$prog/test_script.sh `which $prog` ./$protected > test_${prog}.log 2>&1
	if [ $? -eq 0 ]; then
		echo "TEST ($config) ${prog}: PASS"
		progs_pass="$progs_pass $prog"
	else
		echo "TEST ($config) ${prog}: FAIL"
		progs_fail="$progs_fail $prog"
	fi
done

echo "================================================"
echo "TEST SUMMARY PASS ($config): $progs_pass"
echo "TEST SUMMARY FAIL ($config): $progs_fail"
echo "TEST SUMMARY FAIL ($config) (ps_analyze): $progs_fail_peasoup"

popd

done
