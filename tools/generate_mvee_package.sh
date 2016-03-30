#!/bin/bash

declare -a variant_json_arr
declare -a variant_config_arr

usage()
{

	echo "
Usage:
	generate_mvee_config.sh 

	[(--diehard|--nodiehard)]
	[(--enablenoh|--disablenoh)]
	[(--enablenol|--disablenol)]
	--indir <path_to_variants>
	--outdir <path_to_variants>
	[--args <arguments string in json format> ]
	[--server <servername>
	[--class <atd class>
	[(--help | h )]
"
}

check_opts()
{

	args="\"-k\", \"start\""
	server="APACHE"
	class="None"
	backend="zipr"	 	
	use_diehard="--nodiehard"
	use_noh="--disablenoh"
	use_nol="--disablenol"



        # Note that we use `"$@"' to let each command-line parameter expand to a 
        # separate word. The quotes around `$@' are essential!
        # We need TEMP as the `eval set --' would nuke the return value of getopt.
        short_opts="h"
        long_opts="--long diehard
                   --long nodiehard
		   --long enablenoh
		   --long disablenoh
		   --long enablenol
		   --long disablenol
                   --long indir:
                   --long outdir:
                   --long args:
                   --long server:
                   --long class: 
                   --long help
                   --long zipr
                   --long strata
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
				echo "Setting backend = zipr"
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

	main_exe=$(/bin/ls -F $indir/target_apps/ |egrep "/$"|sed "s|/$||"|sed "s/^dh-//" )
	if [ -d $indir/target_app_libs ]; then
		libraries=$(/bin/ls $indir/target_app_libs/ |grep -v "^dh-lib$"|sed "s/^dh-//")
	fi
	configs=$(/bin/ls $indir/target_apps/dh-$main_exe/)
	echo Found application=\"$main_exe\"
	echo Found libraries=\"$libraries\"
	echo Found configurations=\"$configs\"


	# sanity check that there's only one application.
	if [ $(echo $main_exe|wc -w) -ne 1 ]; then

		echo "Found zero or more than one app in $indir/target_apps/"
		echo "Malformed input.  Aborting...."
		exit 3
	fi


	# sanity check that the configs in the main app match the configs for the libraries.
	for config in $configs; do
		for lib in $libraries; do
		
			if [ ! -d $indir/target_app_libs/dh-$lib/$config ]; then 
				echo "Found that $indir/target_app_libs/dh-$lib/$config is missing."
				echo "Malformed input.  Aborting...."
			fi
		done
	done


	# count the number of variants that will be in the system, and sanity check that each library has those same variants.
	total_variants=0
	
	for config in $configs; do
		for variant_dir in $indir/target_apps/dh-$main_exe/$config/v[0-9]*/
		do
			variant_json_arr[$total_variants]="$variant_dir/peasoup_executable_dir/variant_config.json"
			variant_config_arr[$total_variants]="$config"
			total_variants=$(expr $total_variants + 1)
			if [ ! -f $variant_json ]; then
				echo "Variant configuration file ($variant_json) missingi."
				exit 3
			fi
			variant_jsons="$variant_jsons $variant_json"

			for lib in $libraries; do
				varNum=$(basename $variant_dir)
				if [ ! -d $indir/target_app_libs/dh-$lib/$config/$varNum ]; then
					echo "Found that $indir/target_app_libs/dh-$lib/$config/$varNum is missing"
					exit 3
				fi
			done
		
			
		done
	done

	echo "Found a total of $total_variants to run in parallel."
	echo "Sanity checks complete.  Let's do this..."
}



finalize_json()
{
	# grab the nol/noh stuff
	readarray s_nolnoh < $indir/structured_nolnoh

	mkdir $outdir/global


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

	for seq in $(seq 0 $(expr $total_variants - 1))
	do

		new_variant_dir="$outdir/variant$(expr $seq + 1 )"
		new_variant_dir_ts="/target_apps/variant$(expr $seq + 1 )"
		mkdir $new_variant_dir
		mkdir $new_variant_dir/extra 2>/dev/null || true
		mkdir $new_variant_dir/resources 2>/dev/null || true

		config=${variant_config_arr[$seq]}
		variant_json=${variant_json_arr[$seq]}

		#echo seq=$seq
		#echo config=$config
		#echo variant_json=$variant_json
		echo "Including variant $seq."

		if [ ! -f $variant_json ]; then
			echo wtf
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
			lib_dir="/target_app_libs/dh-$lib/$config/$var_num_dir"

			mkdir -p $new_variant_dir/lib 2>/dev/null || true
			cp  $indir/$lib_dir/$lib $new_variant_dir/lib
			cp -R $indir/$lib_dir/peasoup_executable_dir $new_variant_dir/lib/peasoup_executable_dir.$lib.$config
	
			# note the weird $'\n' is bash's way to encode a new line.
			# set line=  ,\n"alias=file" -- but the bash is ugly, and I can't do better.
			line=",  "$'\n\t\t\t'"  \"/usr/lib/$lib=$new_variant_dir_ts/lib/$lib\" "
			variant_config_contents="${variant_config_contents//,<<LIBS>>/$line,<<LIBS>>}"
	
		done

		if [ "x"$use_nol = "x--enablenol" ]; then
			line=",  "$'\n\t\t\t'"  \"/lib64/ld-linux-x86-64.so.2=/<<EXEPATH>>/peasoup_executable_dir/ld-linux-x86-64.so.2.nol\" "
			variant_config_contents="${variant_config_contents//,<<LIBS>>/$line,<<LIBS>>}"
			variant_config_contents="${variant_config_contents//<<LDLIB>>/$line,<<LDLIB>>}"
		else
			# strata workarounds...
			line=",  "$'\n\t\t\t'"  \"/lib64/ld-linux-x86-64.so.2=/lib64/ld-linux-x86-64.so.2.nol\" "
			variant_config_contents="${variant_config_contents//<<LDLIB>>/$line,<<LDLIB>>}"
		fi

		# handle structured nol/noh
		echo $total_variants > $new_variant_dir/nolnoh_config
		for line in "${s_nolnoh[@]}"
		do
			echo "doing stuff for line $line and vars $seq $config"
			vartag=$(echo $line|cut -d" " -f1)
			varindex=$(echo $line|cut -d" " -f2)
			if [ $seq = $varindex ] && [ $vartag = $config ]; then
				#variant_config_contents="${variant_config_contents//<<ENV>>/VARIANTINDEX=$seq,<<ENV>>}"
				echo $seq >> $new_variant_dir/nolnoh_config
				break
			fi
		done


		variant_name="variant_${seq}"
		variant_config_contents="${variant_config_contents//<<EXEPATH>>/$new_variant_dir_ts\/bin}"
		variant_config_contents="${variant_config_contents//<<VARIANTNUM>>/$variant_name}"
		variant_config_contents="${variant_config_contents//<<VARIANTINDEX>>/$seq}"
		variant_config_contents="${variant_config_contents//<<VARIANTDIR>>/$new_variant_dir_ts}"
		json_contents="${json_contents//<<VARIANT_CONFIG>>/$variant_config_contents,<<VARIANT_CONFIG>>}"
		json_contents="${json_contents//<<VARIANT_LIST>>/\"$variant_name\",<<VARIANT_LIST>>}"


		seq=$(expr $seq + 1)

	done

	if [ $server = "APACHE" ]; then
		ld_preload_var="/thread_libs/libgetpid.so "
	fi
	if [ "x"$use_diehard  = "x--diehard" ]; then
		ld_preload_var="/variant_specific/libheaprand.so $ld_preload_var"

	fi
	if [ "x"$use_noh = "x--enablenoh" ]; then
		ld_preload_var="/variant_specific/noh.so $ld_preload_var"
		#json_contents="${json_contents//<<ENV>>/\"NUMVARIANTS=$total_variants\",<<ENV>>}"
	fi
	# remove leading/trailing spaces.
	ld_preload_var=${ld_preload_var%% }
	ld_preload_var=${ld_preload_var## }

	json_contents="${json_contents//<<ENV>>/\"LD_PRELOAD=$ld_preload_var\",<<ENV>>}"

	# remove variant_config marker.
	json_contents="${json_contents//,<<VARIANT_CONFIG>>/}"
	json_contents="${json_contents//,<<VARIANT_LIST>>/}"
	json_contents="${json_contents//,<<ENV>>/}"
	json_contents="${json_contents//<<ENV>>/}"
	json_contents="${json_contents//,<<LIBS>>/}"
	json_contents="${json_contents//<<LIBS>>/}"
	json_contents="${json_contents//,<<LDLIB>>/}"
	json_contents="${json_contents//<<LDLIB>>/}"
	json_contents="${json_contents//<<CLASS>>/$class}"
	json_contents="${json_contents//<<SERVER>>/$server}"
	json_contents="${json_contents//<<ARGS>>/$args}"

	echo "$json_contents" > $json

	echo "Finalized $json as atd config."
}

main()
{

	check_opts "$@"

	sanity_check

	finalize_json

}

main "$@"
