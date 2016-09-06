#!/bin/bash

declare -a variant_json_arr
declare -a variant_config_arr

usage()
{

	echo "
Usage:
	generate_mvee_config.sh 

	[(--include-cr|--noinclude-cr)]
	[(--diehard|--nodiehard)]
	[(--libtwitcher|--nolibtwitcher)]
	[(--enablenoh|--disablenoh)]
	[(--enablenol|--disablenol)]
	--indir <path_to_variants>
	--outdir <path_to_variants>
	[--args <arguments string in json format> ]
	[--server <servername>
	[--class <atd class>
	[--mainexe <exe base name>
	[(--help | h )]
"
}

check_opts()
{
	echo args="$@"
	# defaults
	args="\"-k\", \"start\""
	server="APACHE"
	class="None"
	backend="zipr"	 	
	use_diehard="--nodiehard"
	use_libtwitcher="--nolibtwitcher"
	use_noh="--disablenoh"
	use_nol="--disablenol"
	use_includecr="--noinclude-cr"
	mainexe_opt="" # look in target_apps and find exactly one thing.



        # Note that we use `"$@"' to let each command-line parameter expand to a 
        # separate word. The quotes around `$@' are essential!
        # We need TEMP as the `eval set --' would nuke the return value of getopt.
        short_opts="h"
        long_opts="
		   --long libtwitcher
                   --long nolibtwitcher
		   --long diehard
                   --long nodiehard
		   --long enablenoh
		   --long disablenoh
		   --long enablenol
		   --long disablenol
		   --long include-cr
		   --long noinclude-cr
                   --long indir:
                   --long outdir:
                   --long args:
                   --long server:
                   --long class: 
                   --long help
                   --long zipr
                   --long strata
                   --long mainexe:
                "

        # solaris does not support long option names
        if [ `uname -s` = "SunOS" ]; then
                TEMP=`getopt $short_opts "$@"`
        else
                TEMP=`getopt -o $short_opts $long_opts -n 'generate_mvee_config.sh' -- "$@"`
        fi

        # error check #
        if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit -1 ; fi

        # Note the quotes around `$TEMP': they are essential!
        eval set -- "$TEMP"

        while true ; do
                case "$1" in
                        --mainexe)
				echo "Setting mainexe = $2"
				mainexe_opt="$2"
                                shift 2
			;;
                        --help|-h)
                                usage;
                                exit 1
                        ;;
                        --indir)
				echo "Setting indir = $2"
                                indir="$2"
                                shift 2
			;;
                        --outdir)
				echo "Setting outdir = $2"
                                outdir="$2"
                                shift 2
			;;
                        --args)
				echo "Setting args = $2"
                                args="$2"
                                shift 2
			;;
                        --class)
                                class="$2"
				echo "Setting class = $2"
                                shift 2
			;;
                        --server)
				echo "Setting server = $2"
                                server="$2"
                                shift 2
			;;
                        --strata)
				echo "Setting backend = strata"
                                backend="strata"
                                shift
			;;
                        --zipr)
				echo "Setting backend = zipr"
                                backend="zipr"
                                shift
			;;
                        --diehard|--nodiehard)
				echo "Setting diehard = $1"
				use_diehard="$1"
                                shift 1
			;;
                        --libtwitcher|--nolibtwitcher)
				echo "Setting libtwitcher = $1"
				use_libtwitcher="$1"
                                shift 1
			;;
                        --include-cr|--noinclude-cr)
				echo "Setting include-cr = $1"
				use_includecr="$1"
                                shift 1
			;;
			--enablenoh|--disablenoh)
				echo "Setting noh = $1"
				use_noh="$1"
				shift 1
			;;
			--enablenol|--disablenol)
				echo "Setting nol = $1"
				use_nol="$1"
				shift 1
			;;
                        --)
                                shift
                                break
                        ;;
                        *)
                                 echo "Internal error!" 
                                echo found option "$1"

                                exit -2
                        ;;
                esac
        done

        # report errors if found
        if [ ! -z $1 ]; then
                echo Unparsed/unimplemented parameters:
                for arg do echo '--> '"\`$arg'" ; done
                exit 3;
        fi

	if [ "x$indir" = "x" ]; then
		echo "Specifying a directory is necessary"
		exit 3;
	fi	

	server=${server^^} # uppercase the server setting.
}


sanity_check()
{
	# count the number of variants that will be in the system, and sanity check that each library has those same variants.
	total_variants=0
	total_variant_sets=0

	var_sets=$(ls $indir)
	for vs_dir in $var_sets
	do
		vs_top_dir=$indir/$vs_dir
		total_variant_sets=$(expr $total_variant_sets + 1)

		if [[ "X$mainexe_opt" = "X" ]]; then
			echo "Trying to infer exe name."
			if [ -d $vs_top_dir/target_apps ]; then
				main_exe=$(/bin/ls -F $vs_top_dir/target_apps/ |egrep "/$"|sed "s|/$||"|sed "s/^dh-//" )
			else
				echo
				echo "Error:  main executable not found in $vs_top_dir/target_apps.  Please use --mainexe      ************************"
				exit 1
			fi
		else
			if [ -d $vs_top_dir/target_apps/dh-$mainexe_opt ]; then
				main_exe=$mainexe_opt
			elif [ -d $vs_top_dir/target_app_libs/dh-$mainexe_opt ]; then
				main_exe=$mainexe_opt
			else
				echo
				echo "Error:  main executable not found in $vs_top_dir/target_apps or $vs_top_dir/target_app_libs.  Please fix --mainexe      ************************"
				exit 1
			fi
		fi



		if [ -d $vs_top_dir/target_app_libs ]; then
			libraries=$(/bin/ls $vs_top_dir/target_app_libs/ |grep -v "^dh-lib$" |sed "s/dh-$main_exe//" |sed "s/^dh-//")
		fi
		configs=$(/bin/ls $vs_top_dir/target_app*/dh-$main_exe/)
		echo "For variant $vs_dir:"
		echo "	Found application=\"$main_exe\""
		echo "	Found libraries=\"$libraries\""
		echo "	Found configurations="\"$configs\"


		# sanity check that there's only one application.
		if [ $(echo $main_exe|wc -w) -ne 1 ]; then

			echo "Found zero or more than one app in $vs_top_dir/target_apps/"
			echo "Malformed input.  Aborting...."
			exit 3
		fi


		# sanity check that the configs in the main app match the configs for the libraries.
		for config in $configs; do
			for lib in $libraries; do
			
				if [ ! -d $vs_top_dir/target_app_libs/dh-$lib/$config ]; then 
					echo "Found that $vs_top_dir/target_app_libs/dh-$lib/$config is missing."
					echo "Malformed input.  Aborting...."
				fi
			done
		done


		# reset		
		variants_per_vs=0

		for config in $configs; do
			for variant_dir in $vs_top_dir/target_app*/dh-$main_exe/$config/v[0-9]*/
			do
				total_variants=$(expr $total_variants + 1)
				variants_per_vs=$(expr $variants_per_vs + 1)
				variant_json_arr[$total_variants]="$variant_dir/peasoup_executable_dir/variant_config.json"
				variant_config_arr[$total_variants]="$config"
				if [ ! -f $variant_json ]; then
					echo "Variant configuration file ($variant_json) missing."
					exit 3
				fi
				variant_jsons="$variant_jsons $variant_json"

				for lib in $libraries; do
					varNum=$(basename $variant_dir)
					if [ ! -d $vs_top_dir/target_app_libs/dh-$lib/$config/$varNum ]; then
						echo "Found that $vs_top_dir/target_app_libs/dh-$lib/$config/$varNum is missing"
						exit 3
					fi
				done
			
				
			done
		done
		echo "For variant set $var_dir:"
		echo " Found a total of $variants_per_vs to run in parallel."
	done

	echo " Sanity checks complete.  Let's do this.... "
}



finalize_json()
{
	mkdir -p $outdir
	mkdir $outdir/global
	mkdir $outdir/marshaling
	mkdir $outdir/marshaling/emt
	cp $CFAR_EMT_PLUGINS/dh_plugins.jar $outdir/marshaling/
	cp $CFAR_EMT_PLUGINS/refresh.py $outdir/marshaling/
	tar -xzf $CFAR_APOGEE_DOWNLOADS_DIR/build-emt.tar.gz -C $outdir/marshaling/emt --strip-components=1


	variants="$total_variants"
	outfile="$outdir/monitor.conf"
	json=${outfile}

	if [ "$backend" = 'zipr' ]; then
		cp $PEASOUP_HOME/tools/cfar_configs/zipr_all.json.template $json
	elif [ "$backend" = 'strata' ]; then
		cp $PEASOUP_HOME/tools/cfar_configs/strata_all.json.template $json
	else
		echo unknown backend: $backend
		exit 1
	fi

	json_contents=$(<$json)

	for vs in $(seq 1 $total_variant_sets)
	do
		vs_json_contents=" \"vs-$vs\" : [ <<VARIANT_LIST>> ]"

		for seq in $(seq 1 $variants_per_vs )
		do

			new_variant_dir="$outdir/vs-$vs/variant-$seq"
			new_variant_dir_ts="/target_apps/vs-$vs/variant-$seq"
			mkdir -p $new_variant_dir
			mkdir $new_variant_dir/extra 2>/dev/null || true
			mkdir $new_variant_dir/resources 2>/dev/null || true

			config=${variant_config_arr[$seq]}
			variant_json=${variant_json_arr[$seq]}

			#echo seq=$seq
			#echo config=$config
			#echo variant_json=$variant_json
			echo "Including variant $seq."

			if [ ! -f $variant_json ]; then
				echo "wtf, $variant_json missing?"
				exit 4
			fi

			


			# start with ps_dir
			ps_dir=$(dirname $variant_json)

			# get path to exe
			full_exe_dir=$(dirname $ps_dir)



			# remove host's portion of the path to get path on target
			exe_dir=$(echo $full_exe_dir|sed "s/^$indir//")

			cp -R $full_exe_dir $new_variant_dir/bin

			# echo "exe_dir=$exe_dir"

			# get the variant number for this config (e.g., get "v0" or "v1")
			var_num_dir=$(basename $exe_dir)


			# read variant config
			variant_config_contents=$(<$variant_json)

			# fill in any libraries that the variants should refer to
			for lib in $libraries
			do
				# echo adding lib $lib
				lib_dir="/vs-$vs/target_app_libs/dh-$lib/$config/$var_num_dir"

				mkdir -p $new_variant_dir/lib 2>/dev/null || true
				cp  $indir/$lib_dir/$lib $new_variant_dir/lib
				cp -R $indir/$lib_dir/peasoup_executable_dir $new_variant_dir/lib/peasoup_executable_dir.$lib.$config
		
				# note the weird $'\n' is bash's way to encode a new line.
				# set line=  ,\n"alias=file" -- but the bash is ugly, and I can't do better.
				line=",  "$'\n\t\t\t'"  \"/usr/lib/$lib=$new_variant_dir_ts/lib/$lib\" "
				variant_config_contents="${variant_config_contents//,<<LIBS>>/$line,<<LIBS>>}"
		
			done

			if [ "x"$use_nol = "x--enablenol" ]; then
				line=",  "$'\n\t\t\t'"  \"<<EXEPATH>>/peasoup_executable_dir/ld-linux-x86-64.so.2.nol=<<EXEPATH>>/peasoup_executable_dir/ld-linux-x86-64.so.2.nol\" "
				variant_config_contents="${variant_config_contents//,<<LIBS>>/$line,<<LIBS>>}"
				variant_config_contents="${variant_config_contents//,<<LDLIB>>/$line,<<LDLIB>>}"
				binname=""
				# ok, now we need the binary name
				if [ "$backend" = 'zipr' ]; then
					# unfortunately, the new way of doing all this relies on the new interp being shorter than the old one, so we need to do this:
					cp $new_variant_dir/bin/peasoup_executable_dir/ld-linux-x86-64.so.2.nol $outdir/ld-nol.so
				elif [ "$backend" = 'strata' ]; then
					# this is still on patchelf due to changing that being complex
					echo "trying patch"
					$CFAR_HOME/non_overlapping_libraries/patchelf --set-interpreter $new_variant_dir_ts/bin/peasoup_executable_dir/ld-linux-x86-64.so.2.nol $new_variant_dir/bin/peasoup_executable_dir/a.stratafied
				fi
			else
				# strata workarounds...
				line=",  "$'\n\t\t\t'"  \"/lib64/ld-linux-x86-64.so.2=/lib64/ld-linux-x86-64.so.2.nol\" "
				variant_config_contents="${variant_config_contents//<<LDLIB>>/$line,<<LDLIB>>}"
			fi

			# handle structured nol/noh
			echo $total_variants > $new_variant_dir/nolnoh_config
			echo config is $config
			if [[ $config == *"structNol"* ]] || [[  $config == *"structNoh"* ]] ; then
				echo $seq >> $new_variant_dir/nolnoh_config
				if [[ $config == *"probNoh"* ]] || [[  $config == *"probNol"* ]] ; then
					echo
					echo "Cannot have structNol with probNoh or structNoh with probNol.  Fatal error. "
					echo
					exit 1
				fi
			fi


			variant_name="variant_${vs}_${seq}"
			variant_config_contents="${variant_config_contents//<<EXEPATH>>/$new_variant_dir_ts\/bin}"
			variant_config_contents="${variant_config_contents//<<VARIANTNUM>>/$variant_name}"
			variant_config_contents="${variant_config_contents//<<VARIANTINDEX>>/$seq}"
			variant_config_contents="${variant_config_contents//<<VARIANTDIR>>/$new_variant_dir_ts}"
			json_contents="${json_contents//<<VARIANT_CONFIG>>/$variant_config_contents,<<VARIANT_CONFIG>>}"

			# only put variant set 1 in the default set.
			if [[ $vs = 1 ]]; then
				json_contents="${json_contents//<<VARIANT_LIST>>/\"$variant_name\",<<VARIANT_LIST>>}"
			fi
			vs_json_contents="${vs_json_contents//<<VARIANT_LIST>>/\"$variant_name\",<<VARIANT_LIST>>}"


			seq=$(expr $seq + 1)

		done
		json_contents="${json_contents//<<VARIANT_SETS>>/$vs_json_contents,<<VARIANT_SETS>>}"
	done

	# copy libpthread_exit.so 
	cp $CFAR_HOME/pthread_exit/libpthread_exit.so $outdir/global/

	if [ $server = "APACHE" ]; then
		ld_preload_var="/thread_libs/libgetpid.so:/thread_libs/libprimaryleads.so:/target_apps/global/libpthread_exit.so"
	fi
	if [ "x"$use_diehard  = "x--diehard" -o  "x"$use_libtwitcher  = "x--libtwitcher" ]; then
		ld_preload_var="/variant_specific/libheaprand.so:$ld_preload_var"

	fi
	if [ "x"$use_noh = "x--enablenoh" ]; then
		ld_preload_var="/variant_specific/noh.so:$ld_preload_var"
		#json_contents="${json_contents//<<ENV>>/\"NUMVARIANTS=$total_variants\",<<ENV>>}"
	fi
	# remove leading/trailing spaces.
	ld_preload_var=${ld_preload_var%% }
	ld_preload_var=${ld_preload_var## }

	json_contents="${json_contents//<<ENV>>/\"LD_PRELOAD=$ld_preload_var\",<<ENV>>}"

	# if we are supposed to include checkpoint/restore lines in the file
	if [ "x"$use_includecr = "x--include-cr" ]; then
		echo "Including C/R support."
		# grab the appropriate contents for cp/rest and monitor settings.
		cr_contents=$(cat $PEASOUP_HOME/tools/cfar_configs/cr_chunk.json.template)
		monitor_contents=$(cat $PEASOUP_HOME/tools/cfar_configs/monitor_chunk_with_cr.json.template)

		#echo setting monitor chunk to: $monitor_contents
		json_contents="${json_contents//<<CR>>/$cr_contents}"
		json_contents="${json_contents//<<MONITOR>>/$monitor_contents}"
	else
		echo "Not doing C/R support."
		# grab the appropriate contents monitor settings and remove c/r marker
		monitor_contents=$(cat $PEASOUP_HOME/tools/cfar_configs/monitor_chunk.json.template)
		#echo setting monitor chunk to: $monitor_contents
		json_contents="${json_contents//<<CR>>/}"
		json_contents="${json_contents//<<MONITOR>>/$monitor_contents}"
	fi

	# substittue in the right settings for the monitor settings.

	# remove variant_config marker.
	json_contents="${json_contents//,<<VARIANT_CONFIG>>/}"
	json_contents="${json_contents//,<<VARIANT_LIST>>/}"
	json_contents="${json_contents//,<<VARIANT_SETS>>/}"
	json_contents="${json_contents//,<<ENV>>/}"
	json_contents="${json_contents//<<ENV>>/}"
	json_contents="${json_contents//,<<LIBS>>/}"
	json_contents="${json_contents//<<LIBS>>/}"
	json_contents="${json_contents//,<<LDLIB>>/}"
	json_contents="${json_contents//<<LDLIB>>/}"
	json_contents="${json_contents//<<CLASS>>/$class}"
	json_contents="${json_contents//<<SERVER>>/$server}"
	json_contents="${json_contents//<<ARGS>>/$args}"

	echo "$json_contents" |json_pp > $json

	echo "Finalized $json as atd config."
}

main()
{

	check_opts "$@"

	sanity_check

	finalize_json

}

main "$@"
