#!/bin/bash

declare -a variant_json_arr
declare -a variant_config_arr

usage()
{

	echo "
Usage:
	generate_mvee_config.sh 

	[(--diehard|--no-diehard)]
	--dir <path_to_variants>
	[--args <arguments string in json format> ]
	[--server <servername>
	[--class <atd class>
	[(--help | h )]
"
}

check_opts()
{

	args="\"-k\", \"start\""
	server="Apache"
	class="None"
	use_diehard="--nodiehard"



        # Note that we use `"$@"' to let each command-line parameter expand to a 
        # separate word. The quotes around `$@' are essential!
        # We need TEMP as the `eval set --' would nuke the return value of getopt.
        short_opts="h"
        long_opts="--long diehard
                   --long nodiehard
                   --long dir:
                   --long args:
                   --long server:
                   --long class: 
                   --long help
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
                        --dir)
				echo "Setting dir = $2"
                                dir="$2"
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
                        --diehard|--nodiehard)
				echo "Setting diehard = $1"
				use_diehard="$1"
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

	if [ "x$dir" = "x" ]; then
		echo "Specifying a directory is necessary"
		exit 3;
	fi	
}


sanity_check()
{

	main_exe=$(/bin/ls -F $dir/target_apps/ |egrep "/$"|sed "s|/$||"|sed "s/^dh-//" )
	libraries=$(/bin/ls $dir/target_app_libs/ |grep -v "^dh-lib$"|sed "s/^dh-//")
	configs=$(/bin/ls $dir/target_apps/dh-$main_exe/)
	echo Found application=\"$main_exe\"
	echo Found libraries=\"$libraries\"
	echo Found configurations=\"$configs\"


	# sanity check that there's only one application.
	if [ $(echo $main_exe|wc -w) -ne 1 ]; then

		echo "Found zero or more than one app in $dir/target_apps/"
		echo "Malformed input.  Aborting...."
		exit 3
	fi


	# sanity check that the configs in the main app match the configs for the libraries.
	for config in $configs; do
		for lib in $libraries; do
		
			if [ ! -d $dir/target_app_libs/dh-$lib/$config ]; then 
				echo "Found that $dir/target_app_libs/dh-$lib/$config is missing."
				echo "Malformed input.  Aborting...."
			fi
		done
	done


	# count the number of variants that will be in the system, and sanity check that each library has those same variants.
	total_variants=0
	
	for config in $configs; do
		for variant_dir in $dir/target_apps/dh-$main_exe/$config/v[0-9]*/
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
				if [ ! -d $dir/target_app_libs/dh-$lib/$config/$varNum ]; then
					echo "Found that $dir/target_app_libs/dh-$lib/$config/$varNum is missing"
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


	variants="$total_variants"
	outfile="$dir/target_apps/$main_exe.conf"
	backend="zipr"	 # doesn't work with strata yet
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
		exe_dir=$(dirname $ps_dir)


		# remove host's portion of the path to get path on target
		exe_dir=$(echo $exe_dir|sed "s/^$dir//")

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
	
			# note the weird $'\n' is bash's way to encode a new line.
			# set line=  ,\n"alias=file" -- but the bash is ugly, and I can't do better.
			line=",  "$'\n\t\t\t'"  \"/usr/lib/$lib=$lib_dir/$lib\" "
			variant_config_contents="${variant_config_contents//,<<LIBS>>/$line,<<LIBS>>}"
	
		done



		variant_name="variant_${seq}"
		variant_config_contents="${variant_config_contents//<<EXEPATH>>/$exe_dir}"
		variant_config_contents="${variant_config_contents//<<VARIANTNUM>>/$variant_name}"
		json_contents="${json_contents//<<VARIANT_CONFIG>>/$variant_config_contents,<<VARIANT_CONFIG>>}"
		json_contents="${json_contents//<<VARIANT_LIST>>/\"$variant_name\",<<VARIANT_LIST>>}"


		seq=$(expr $seq + 1)

	done

	if [ $server = "Apache" ]; then
		ld_preload_var="/thread_libs/libgetpid.so "
	fi
	if [ "x"$use_diehard  = "x--diehard" ]; then
		ld_preload_var="/variant_specific/libheaprand.so $ld_preload_var"

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
