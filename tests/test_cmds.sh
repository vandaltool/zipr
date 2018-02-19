#!/bin/bash

# specify configs here
#configs="zipr scdi scfi scfi.color scfi.color.nojmps kill_deads"
#configs="scfi.color"
#configs="scfi.color.nojmps"
#configs="scfi.color scfi.color.nojmps"
#configs="shadow"
#configs="shadow.scfi"
#configs="scfi.shadow shadow.scfi"
#configs="scfi.color"
#configs="zipr scfi scfi.color"
#configs="shadow"
configs="p1 scfi"
#configs="ibtl"
#configs="killdeads_strata"
#configs="ibtl ibtl_p1"
#configs="zipr scfi p1"

# specify programs to test
#orig_progs="grep ncal bzip2 du ls objdump readelf sort tar touch tcpdump"
#orig_progs="grep bzip2 du ls objdump readelf sort tar touch"
orig_progs="grep bzip2 du ls"
#orig_progs="bzip2"
#orig_progs="gedit"
#orig_progs="gimp"
#orig_progs="openssl"
#orig_progs="tcpdump sort touch"
#orig_progs="grep"
build_only=0

export IB_VERBOSE=1

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

	progpath=$(which $prog)
	progpath=$(readlink -f $progpath)
        
	case $config in
		zipr)
			$PSZ $progpath $protected --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		p1)
			$PSZ $progpath $protected --step p1transform=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi.color)
			FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend zipr --step selective_cfi=on --step-option selective_cfi:--color --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi.color.nojmps)
			FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend zipr --step selective_cfi=on --step-option selective_cfi:--color --step-option selective_cfi:--no-protect-jumps --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected --step selective_cfi=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		kill_deads)
			$PSZ $progpath $protected --step kill_deads=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scdi)
			SimpleCDI_VERBOSE=1 $PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend zipr --step simple_cdi=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		shadow)
			$PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend zipr --step fptr_shadow=on --step-option "zipr:--zipr:callbacks $ZIPR_INSTALL/bin/callbacks.datashadow.exe" --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		shadow.scfi)
			FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend zipr --step fptr_shadow=on --step selective_cfi=on --step-option "zipr:--zipr:callbacks $ZIPR_INSTALL/bin/callbacks.datashadow.exe" --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi.shadow)
			echo "make copy of fptr_shadow.exe --> shadow.exe to force the shadow step to occur after the scfi step"
			cp $SECURITY_TRANSFORMS_HOME/plugins_install/fptr_shadow.exe $SECURITY_TRANSFORMS_HOME/plugins_install/shadow.exe
			FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend zipr --step shadow=on --step selective_cfi=on --step-option "zipr:--zipr:callbacks $ZIPR_INSTALL/bin/callbacks.datashadow.exe" --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		killdeads_strata)
			$PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend strata --step ibtl=on --step ilr=on --step kill_deads=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		ibtl)
			$PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend strata --step ibtl=on --step ilr=on --step pc_confine=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		ibtl_p1)
			$PEASOUP_HOME/tools/ps_analyze.sh $progpath $protected --backend strata --step ibtl=on --step ilr=on --step pc_confine=on --step p1transform=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		*)
			echo "Unknown configuration requested"
			continue
		;;
	esac

	if [ ! $? -eq 0 ]; then
		echo "TEST ($config) ${prog}: FAILED to peasoupify"
		progs_fail_peasoup="$progs_fail_peasoup $prog"
    else
		progs_pass_peasoup="$progs_pass_peasoup $prog"
	fi

	if [ $build_only -eq 1 ]; then
		continue
	fi

	echo "TEST ($config) ${prog}: Running tests..."
	timeout 300 ../$prog/test_script.sh $progpath ./$protected > test_${prog}.log 2>&1
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
echo "TEST SUMMARY PASS ($config) (ps_analyze): $progs_pass_peasoup"
echo "TEST SUMMARY FAIL ($config): $progs_fail"
echo "TEST SUMMARY FAIL ($config) (ps_analyze): $progs_fail_peasoup"

popd

done
