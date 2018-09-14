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
#configs="p1 scfi"
#configs="mg mgx mgx_p1 p1_mgx"
#configs="ibtl"
#configs="killdeads_strata"
#configs="ibtl ibtl_p1"
#configs="zipr scfi p1"
configs="zafl_rida"
configs="zafl_ida zafl_rida zafl_ida_nostars zafl_rida_nostars"
configs="zafl"

# specify programs to test
orig_progs="bzip2 grep du ncal ls objdump readelf sort tar touch tcpdump"
build_only=0

export IB_VERBOSE=1

for config in $configs
do

echo "----------------------------------"
echo "TEST CONFIGURATION: $config"
progs_pass=""
progs_pass_peasoup=""
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


	progpath=$(which $prog)
	if [ ! -e $progpath ]; then
		echo "$prog not found: skipping..."
		continue
	fi

	echo "TEST ($config) ${prog}: Protecting..."

	progpath=$(readlink -f $progpath)
        
	case $config in
		zafl)
			zafl.sh $progpath $protected --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		zafl_rida)
			zafl.sh $progpath $protected --rida --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		zafl_rida_nostars)
			zafl.sh $progpath $protected --rida --no-stars --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		zafl_ida)
			zafl.sh $progpath $protected --ida --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		zafl_ida_nostars)
			zafl.sh $progpath $protected --ida --no-stars --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		zafl_rida)
			zafl.sh $progpath $protected --rida --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		zipr)
			$PSZ $progpath $protected --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		rida)
			$PSZ $progpath $protected -s meds_static=off -s rida=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		rida_p1)
			$PSZ $progpath $protected -s meds_static=off -s rida=on -s p1transform=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		rida_scfi)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected -s meds_static=off -s rida=on -s selective_cfi=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		rida_p1_scfi)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected -s meds_static=off -s rida=on -s p1transform=on -s selective_cfi=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		p1)
			$PSZ $progpath $protected --step p1transform=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		mg)
			$PSZ $progpath $protected --step move_globals=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		mg_elfonly)
			$PSZ $progpath $protected --step move_globals=on -o move_globals:--elftables-only --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		mgx)
			$PSZ $progpath $protected --step move_globals=on --step-option move_globals:--aggressive --step xor_globals=on  --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		mgx_p1)
			$PSZ $progpath $protected --step move_globals=on --step-option move_globals:--aggressive --step xor_globals=on --step p1transform=on  --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		p1_mgx)
			$PSZ $progpath $protected --step p1transform=on --step move_globals=on --step-option move_globals:--aggressive --step xor_globals=on --step p1transform=on  --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi.color)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected --backend zipr --step selective_cfi=on --step-option selective_cfi:--color --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi.color.nojmps)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected --backend zipr --step selective_cfi=on --step-option selective_cfi:--color --step-option selective_cfi:--no-protect-jumps --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected --step selective_cfi=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		p1_scfi)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected -c p1transform=on -c selective_cfi=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		p1_scfi_mg)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected -c p1transform=on -c selective_cfi=on -c move_globals=on --step-option move_globals:--aggressive --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		kill_deads)
			$PSZ $progpath $protected --step kill_deads=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scdi)
			SimpleCDI_VERBOSE=1 $PSZ $progpath $protected --backend zipr --step simple_cdi=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		shadow)
			$PSZ $progpath $protected --backend zipr --step fptr_shadow=on --step-option "zipr:--zipr:callbacks $ZIPR_INSTALL/bin/callbacks.datashadow.exe" --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		shadow.scfi)
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected --backend zipr --step fptr_shadow=on --step selective_cfi=on --step-option "zipr:--zipr:callbacks $ZIPR_INSTALL/bin/callbacks.datashadow.exe" --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		scfi.shadow)
			echo "make copy of fptr_shadow.exe --> shadow.exe to force the shadow step to occur after the scfi step"
			cp $SECURITY_TRANSFORMS_HOME/plugins_install/fptr_shadow.exe $SECURITY_TRANSFORMS_HOME/plugins_install/shadow.exe
			FIX_CALLS_FIX_ALL_CALLS=1 $PSZ $progpath $protected --backend zipr --step shadow=on --step selective_cfi=on --step-option "zipr:--zipr:callbacks $ZIPR_INSTALL/bin/callbacks.datashadow.exe" --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		killdeads_strata)
			$PSZ $progpath $protected --backend strata --step ibtl=on --step ilr=on --step kill_deads=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		ibtl)
			$PSZ $progpath $protected --backend strata --step ibtl=on --step ilr=on --step pc_confine=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		ibtl_p1)
			$PSZ $progpath $protected --backend strata --step ibtl=on --step ilr=on --step pc_confine=on --step p1transform=on --tempdir $temp_dir > test_${prog}.ps.log 2>&1
		;;
		*)
			echo "Unknown configuration requested"
			continue
		;;
	esac

	if [ ! $? -eq 0 ]; then
		echo "TEST ($config) ${prog}: FAILED to peasoupify"
		progs_fail_peasoup="$progs_fail_peasoup $prog.$config"
    else
		progs_pass_peasoup="$progs_pass_peasoup $prog.$config"
	fi

	if [ $build_only -eq 1 ]; then
		continue
	fi

	echo "TEST ($config) ${prog}: Running tests..."
	timeout 300 ../$prog/test_script.sh $progpath ./$protected > test_${prog}.log 2>&1
	if [ $? -eq 0 ]; then
		echo "TEST ($config) ${prog}: PASS"
		progs_pass="$progs_pass $prog.$config"
	else
		echo "TEST ($config) ${prog}: FAIL"
		progs_fail="$progs_fail $prog.$config"
	fi
done

echo
echo "================================================"
echo "TEST SUMMARY PASS (build): $progs_pass_peasoup"
echo "TEST SUMMARY PASS (functional): $progs_pass"
echo "TEST SUMMARY FAIL (build): $progs_fail_peasoup"
echo "TEST SUMMARY FAIL (functional): $progs_fail"

popd

done
