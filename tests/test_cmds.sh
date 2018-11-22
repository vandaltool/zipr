#!/bin/bash

show_logs_on_failure=0
had_fails=0
apps=""
default_apps="bzip2 grep du ncal ls objdump readelf sort tar touch tcpdump"
configs=""
default_configs="rida"

do_tests()
{
	local configs="$1"
	local progs_to_test="$2"

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

		for prog in $progs_to_test
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
			if [ "$progpath" = "" ]; then
				echo "TEST: Original binary ($prog) not found: skipping..."
				continue
			fi

			echo "TEST ($config) ${prog}: Protecting..."

			progpath=$(readlink -f $progpath)
			
			case $config in
				zafl)
					zafl.sh $progpath $protected --tempdir $temp_dir > test_${prog}.ps.log 2>&1
				;;
				zafl0)
					ZAFL_LIMIT_END=0 zafl.sh $progpath $protected --tempdir $temp_dir > test_${prog}.ps.log 2>&1
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
				ida)
					$PSZ $progpath $protected -s meds_static=on -s rida=off --tempdir $temp_dir > test_${prog}.ps.log 2>&1
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
				orig)
					cp $progpath $protected 
				;;
				fail)
					set -x
					base_prog=$(basename $progpath)
					if [ $base_prog = "ls" ]; then
						cp $(which diff) $protected 
					else
						cp $(which ls) $protected 
					fi
				;;
				*)
					echo "Unknown configuration requested"
					continue
				;;
			esac

			if [ ! $? -eq 0 ]; then
				echo "TEST ($config) ${prog}: FAILED to peasoupify"
				progs_fail_peasoup="$progs_fail_peasoup $prog.$config"
				cat test_${prog}.ps.log
				if [ $show_logs_on_failure -eq 1 ]; then
					cat $temp_dir/logs/*.log
				fi
		        else
				progs_pass_peasoup="$progs_pass_peasoup $prog.$config"
			fi

			if [ $build_only -eq 1 ]; then
				continue
			fi

			echo "TEST ($config) ${prog}: Running tests..."
			TEST_VERBOSE=1 timeout 300 ../$prog/test_script.sh $progpath ./$protected > test_${prog}.log 2>&1
			if [ $? -eq 0 ]; then
				echo "TEST ($config) ${prog}: PASS"
				progs_pass="$progs_pass $prog.$config"
			else
				echo "TEST ($config) ${prog}: FAIL"
				progs_fail="$progs_fail $prog.$config"
				if [ $show_logs_on_failure -eq 1 ]; then
					cat test_${prog}.log
				fi
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

	if [[ $progs_fail != "" ]] || [[ $progs_fail_peasoup != "" ]]; then
		had_fails=1
	fi
}

usage()
{
	echo "test_cmds.sh [options]"
	echo	
	echo "options:"
	echo "   -h, --help              print help info"
	echo "   -l                      print logs on failure"
	echo "   -a, --app <appname>     app to test (may be repeated)"
	echo "      appname: bzip2 grep du ncal ls objdump readelf sort tar touch tcpdump"
	echo "   -c, --config <config>"
	echo
	echo "config:"
	echo "    rida                   (default configuration)"
	echo "    orig                   test against self"
	echo "    fail                   deliberately induce failure"
	echo "    p1                     P1 transform"
	echo "    mgx                    move globals"
	echo "    mgx_p1                 move globals, followed by P1"
	echo "    kill_deads             xform with bogus random values for dead registers"
	echo
	echo "    ...too many other configs to list, consult script"
}

parse_args()
{
	PARAMS=""
	while (( "$#" )); do
		key="$1"

		case $key in
			-h|--help)
				usage
				exit 0
				;;
			-l)
				show_logs_on_failure=1
				shift
				;;
			-a|--app)
				apps="$apps $2 "
				shift 2
				;;
			-c|--config)
				configs="$configs $2 "
				shift 2
				;;
			-*|--*=) # unsupported flags
				echo "Error: Unsupported flag $1" >&2
				exit 1
				;;
    			*) # preserve positional arguments
				PARAMS="$PARAMS $1"
				shift
				;;
		esac
	done

	eval set -- "$PARAMS"

	# backward compatibility where all configs passed as positional parameters
	# should really use -c option only

	positional=($PARAMS)

	if [[ $positional != "" ]]; then
		configs="$configs $positional"
	fi

	if [[ $configs == "" ]]; then
		configs=" $default_configs"
	fi

	if [[ $apps == "" ]]; then
		apps=" $default_apps"
	fi

}

main()
{
	parse_args "$@"

	echo 
	echo "test configuration"
	echo "    apps: $apps"
	echo " configs: $configs"
	if [ $show_logs_on_failure -eq 1 ]; then
		echo "test log:  show-on-failure"
	fi
	echo 

	do_tests "$configs" "$apps"

	exit $had_fails
}

main "$@"
