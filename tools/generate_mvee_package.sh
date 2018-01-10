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
	[--class <atd class> ]
	[--mainexe <exe base name>
	[--extra_preloads "<preloads>" ]
	[(--help | -h )]
	[(--verbose | -v )]
"
}

check_opts()
{
	# echo args="$@"
	# defaults
	args="\"-k\", \"start\""
	server="APACHE"
	class=""
	backend="zipr"	 	
	use_diehard="--nodiehard"
	use_libtwitcher="--nolibtwitcher"
	use_noh="--disablenoh"
	use_nol="--disablenol"
	use_includecr="--noinclude-cr"
	mainexe_opt="" # look in target_apps and find exactly one thing.
	verbose=0



        # Note that we use `"$@"' to let each command-line parameter expand to a 
        # separate word. The quotes around `$@' are essential!
        # We need TEMP as the `eval set --' would nuke the return value of getopt.
        short_opts="hv"
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
                   --long extra_preloads:
                   --long verbose
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
                        --extra_preloads)
				extra_preloads="$2"
                                shift 2
			;;
                        --mainexe)
				mainexe_opt="$2"
                                shift 2
			;;
                        --verbose|-v)
				verbose=1
				shift 1
			;;
                        --help|-h)
                                usage;
                                exit 1
                        ;;
                        --indir)
                                indir="$2"
                                shift 2
			;;
                        --outdir)
                                outdir="$2"
                                shift 2
			;;
                        --args)
                                args="$2"
                                shift 2
			;;
                        --class)
                                class="$2"
                                shift 2
			;;
                        --server)
                                server="$2"
                                shift 2
			;;
                        --strata)
                                backend="strata"
                                shift
			;;
                        --zipr)
                                backend="zipr"
                                shift
			;;
                        --diehard|--nodiehard)
				use_diehard="$1"
                                shift 1
			;;
                        --libtwitcher|--nolibtwitcher)
				use_libtwitcher="$1"
                                shift 1
			;;
                        --include-cr|--noinclude-cr)
				use_includecr="$1"
                                shift 1
			;;
			--enablenoh|--disablenoh)
				use_noh="$1"
				shift 1
			;;
			--enablenol|--disablenol)
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

	if [[ $verbose = 1 ]]
	then
		echo "Setting preloads = $extra_preloads"
		echo "Setting mainexe = $mainexe_opt"
		echo "Setting indir = $indir"
		echo "Setting outdir = $outdir"
		echo "Setting args = $args"
		echo "Setting class = $class"
		echo "Setting server = $server"
		echo "Setting backend = $backend"
		echo "Setting diehard = $use_diehard"
		echo "Setting libtwitcher = $use_libtwitcher"
		echo "Setting include-cr = $use_includecr"
		echo "Setting noh = $use_noh"
		echo "Setting nol = $use_nol"
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
		echo "For variant set $vs_dir:"
		echo "	Found application=\"$main_exe\""
		echo "	Found libraries=\"$(echo $libraries)\""
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
		echo "	Found a total of $variants_per_vs to run in parallel."
	done

	echo "-------------------------------------------"
	echo "Sanity checks complete.  Let's do this.... "
	echo "-------------------------------------------"
}

copy_stuff()
{
	in=$1
	out=$2
	exe=$3
	target_path=$4
	is_main=$5
	echo -n "	Copying files for $exe ... "

	mkdir -p $out 2> /dev/null

	if [[ $is_main == 1 ]] ; then
		cp $in/*.so 		$out/ 2> /dev/null 
		cp $in/*json 		$out/ 2> /dev/null 
		cp $in/*nol 		$out/ 2> /dev/null 
		cp $in/ld-nol.so.ubuntu $out/ 2> /dev/null
		cp $in/ld-nol.so.centos $out/ 2> /dev/null
	fi
	cp $in/*.map 		$out/ 2> /dev/null 

	new_val="\"$exe\" : \"$target_path/zipr.map\" ";
	variant_config_contents="${variant_config_contents//<<CODE_MAP>>/$new_val, <<CODE_MAP>>}"

	new_val="\"$exe\" : \"$target_path/scoop.map\" ";
	variant_config_contents="${variant_config_contents//<<SCOOP_MAP>>/$new_val, <<SCOOP_MAP>>}"

	new_val="\"$exe\" : \"$target_path/p1.map\" ";
	variant_config_contents="${variant_config_contents//<<P1_MAP>>/$new_val, <<P1_MAP>>}"

	echo  "Done!"

}

# parse assurance evidence into human readable format
# $1 input file (assurance evidence)
# $2 output file (vs-?_variant-?_evidence.txt)
parse_assurance_file()
{
	input=$1
	output=$2

	# find the part of the line that is the transform name, strip out the ASSURANCE_ tag 
	# The space is important to distinguish between variant set AGGREGATE_ASSURANCE and 
	# per_variant_ASSURANCE
	transform_names=`grep [[:space:]]ASSURANCE_ $input | grep :: | sed 's/ASSURANCE_//g' | sed 's/^+.*//g'| sed 's/::.*//g' | uniq`

	# count the number of different transform labels
	j=0
	for i in $transform_names
	do
        	j=`expr $j + 1`
	done

	# for each transform_name find the lines that match the transform, parse them,
	# and place them in the output file
	# simulate outline numbering using 1,2,3... and a,b,c...
	count=1
	for t in $transform_names
	do
		# Remove any underscores and replace with spaces to make more human-readable
        	echo "${count}. Transform Name:  `echo $t | sed 's/_/ /g' `" >> $output

		# The space is important to distinguish between variant set AGGREGATE_ASSURANCE and 
		# per_variant_ASSURANCE
        	matching_lines=`grep [[:space:]]ASSURANCE_ $input | grep :: | sed 's/^+.*//g' | grep $t`

		# starting letter for labelling
		letter=a
        	for m in $matching_lines
        	do
                	echo -n -e "\t${letter}. " >> $output
			# find the stats and print them in human-readable format
			
			# get rid of everything before ::
			# change the = to :
			# change the underscores to spaces
                	echo $m | sed 's/^.*:://g' | sed 's/=/: /g' | sed 's/_/ /g'  >> $output
			# "increment" the letter level
			letter=$(echo "$letter" | tr "0-9a-z" "1-9a-z_")
        	done
		# prettier formatting, add blank line
		echo >> $output
		# increment the counter level
        	count=`expr $count + 1`
	done	
}

# copy assurance evidence to the generated mvee package
copy_assurance_evidence()
{
	# This should be the assurance_evidence log for the binary file
	in=$1
	# This should be the file whose name is vs-?_variant-?_evidence.txt
	out=$2
	#  This is the name of the binary
	exe=$3
	is_main=$4
	transform_config_name=$5 

	echo -n "	Copying assurance evidence file for $exe ... "

	# Don't do anything if there isn't a source file.
	if [[ ! -f "$in" ]]; then
		echo "No assurance case evidence found for $exe config: $transform_config_name."
		return
	fi

	# We will assume that the main exe is handled first, so create the file if is_main is 1
	if [[ $is_main == 1 ]] ; then
		# copy to new file
		echo "Binary Name: $exe" > $out
		echo "Transforms configuration:  $transform_config_name" >> $out
		echo >> $out
		parse_assurance_file $in $out
#		cat $in | grep '^[[:space:]]' >> $out

	else
		# Append to existing file
		echo "Binary Name: $exe" >> $out
		echo "Transforms configuration:  $transform_config_name" >> $out
		echo >> $out
		parse_assurance_file $in $out
#		cat $in | grep '^[[:space:]]' >> $out
	fi

	echo >> $out

	echo "Done copying assurance evidence for $exe!"
}



finalize_json()
{
	# make directories
	mkdir -p $outdir
	mkdir $outdir/global
	mkdir $outdir/marshaling
	mkdir $outdir/assurance

	# copy jar, python, and bash scripts into package.
	cp $CFAR_EMT_PLUGINS/*.jar $outdir/marshaling/
	cp $CFAR_EMT_PLUGINS/*.py $outdir/marshaling/
	cp $CFAR_EMT_PLUGINS/*.sh $outdir/marshaling/

	# copy distribution.
	cp -R $CFAR_DIST_EMT_DIR $outdir/marshaling/emt

	# get any changes from the uva branch
	cp $CFAR_EMT_DIR/*.jar $outdir/marshaling/emt


	# shorthand for certain things. 
	variants="$total_variants"
	outfile="$outdir/monitor.conf"
	json=${outfile}


	# get the tempalte and put it in place.
	if [ "$backend" = 'zipr' ]; then
		cp $PEASOUP_HOME/tools/cfar_configs/zipr_all.json.template $json
	elif [ "$backend" = 'strata' ]; then
		cp $PEASOUP_HOME/tools/cfar_configs/strata_all.json.template $json
	else
		echo "unknown backend: $backend"
		exit 1
	fi

	# read the template and put it into a variable
	json_contents=$(<$json)

	# count how many variants we create in total.
	total_variants=0

	# for each variant set
	for vs in $(seq 1 $total_variant_sets)
	do

		# initialize the vs_json_contents variable.
		# to hold the initial list for the variant set description.
		vs_json_contents=" \"vs-$vs\" : [ <<VARIANT_LIST>> ]"

		# for each varjiant in the variant set.
		for seq in $(seq 1 $variants_per_vs )
		do

			# update count.
			total_variants=$(expr $total_variants + 1)

			# read in the configuration name and the contents of the variant's json file that was memorized
			# during the sanity checking.
			config=${variant_config_arr[$total_variants]}
			variant_json=${variant_json_arr[$total_variants]}


			# update user we're starting a new variant.
			echo "Including variant $seq (variant_${vs}_${seq}) of type $config."

			# calculate where the variantw ill go on both the generation box and the output box.
			new_variant_dir="$outdir/vs-$vs/variant-$seq"
			new_variant_dir_ts="/target_apps/vs-$vs/variant-$seq"

			# make the appropriate subdirs.	
			mkdir -p $new_variant_dir
			mkdir $new_variant_dir/extra 2>/dev/null || true
			mkdir $new_variant_dir/resources 2>/dev/null || true

			#
			# sanity check that nol/noh configuration settings match the config name.
			#
			if [[ $config == *"Noh"* ]]  || [[ $config == *"phase1"* ]] ; then
				if [[ $use_noh == "--enablenoh" ]] ; then
					#echo "	noh settings match."
					echo -n 
				else
					echo
					echo "--enablenoh setting does not match, config is Noh, use_noh is off"
					exit 1
				fi
			else 
				if [[ $use_noh == "--disablenoh" ]] ; then
					#echo "	noh settings match."
					echo -n 
				else
					echo
					echo "--enablenoh setting does not match, config is not noh, use_noh is on"
					exit 1
				fi
			fi
			if [[ $config == *"Nol"* ]]  || [[ $config == *"phase1"* ]] ; then
				if [[ $use_nol == "--enablenol" ]] ; then
					#echo "	--enablenol settings match."
					echo -n 
				else
					echo
					echo "--enablenol setting does not match"
					exit 1
				fi
			else 
				if [[ $use_nol == "--disablenol" ]] ; then
					#echo "	--enablenol settings match."
					echo -n 
				else
					echo
					echo "--enablenol setting does not match"
					exit 1
				fi
			fi

			#echo seq=$seq
			#echo config=$config
			#echo variant_json=$variant_json

			# make sure we have a .json file.
			if [ ! -f $variant_json ]; then
				echo "Error $variant_json missing.  Did protection process fail?"
				exit 4
			fi

			


			# start with ps_dir
			ps_dir=$(dirname $variant_json)

			# get path to exe
			full_exe_dir=$(dirname $ps_dir)

			# get path to exe
			struct_set_dir=$(dirname $full_exe_dir)

			# figure out how many variants in the structured set.
			struct_set_size=$(ls $struct_set_dir |wc -l)
			struct_set_no=$(basename $full_exe_dir |sed "s/v//")


			# remove host's portion of the path to get path on target
			exe_dir=$(echo $full_exe_dir|sed "s|^$indir||")

			# update user.
			echo "	Found variant at $full_exe_dir "

			# read variant config
			variant_config_contents=$(<$variant_json)

			# create output for main exe
			mkdir -p $new_variant_dir/bin
			mkdir -p $new_variant_dir/bin/peasoup_executable_dir/
			cp $full_exe_dir/$main_exe $new_variant_dir/bin

			# new_variant_dir_ts="/target_apps/vs-$vs/variant-$seq"
			copy_stuff $full_exe_dir/peasoup_executable_dir $new_variant_dir/bin/peasoup_executable_dir $main_exe $new_variant_dir_ts/bin/peasoup_executable_dir 1
				
			# copy assurance evidence
			copy_assurance_evidence $full_exe_dir/peasoup_executable_dir/logs/assurance_case_evidence.log  $outdir/assurance/vs-${vs}_variant-${seq}_evidence.txt $main_exe 1 $config


			# echo "exe_dir=$exe_dir"

			# get the variant number for this config (e.g., get "v0" or "v1")
			var_num_dir=$(basename $exe_dir)



			# fill in any libraries that the variants should refer to
			for lib in $libraries
			do
				# echo adding lib $lib
				lib_dir="/vs-$vs/target_app_libs/dh-$lib/$config/$var_num_dir"


				if [ "$main_exe" ==  "$lib" ]; then
					#cp -R $indir/$lib_dir/peasoup_executable_dir $new_variant_dir/lib/peasoup_executable_dir.$lib.$config
					mkdir -p $new_variant_dir/lib/peasoup_executable_dir.$lib.$config
					cp -R $indir/$lib_dir/peasoup_executable_dir/*.so $new_variant_dir/lib/peasoup_executable_dir.$lib.$config/
					cp -R $indir/$lib_dir/peasoup_executable_dir/*config $new_variant_dir/lib/peasoup_executable_dir.$lib.$config/
				fi
		
				# note the weird $'\n' is bash's way to encode a new line.
				# set line=  ,\n"alias=file" -- but the bash is ugly, and I can't do better.
				if [[ $lib == mod* ]]; then
					mkdir -p $new_variant_dir/modules 2>/dev/null || true
					cp  $indir/$lib_dir/$lib $new_variant_dir/modules
					line=",  "$'\n\t\t\t'"  \"/testing/content/apache_support/modules/$lib=$new_variant_dir_ts/modules/$lib\" "
					copy_stuff $indir/$lib_dir/peasoup_executable_dir $new_variant_dir/modules/$lib-peasoup_executable_dir $lib $new_variant_dir_ts/modules/$lib-peasoup_executable_dir 0

					# copy assurance evidence
					copy_assurance_evidence $indir/$lib_dir/peasoup_executable_dir/logs/assurance_case_evidence.log  $outdir/assurance/vs-${vs}_variant-${seq}_evidence.txt $lib 0 $config
				else
					mkdir -p $new_variant_dir/lib 2>/dev/null || true
					cp  $indir/$lib_dir/$lib $new_variant_dir/lib
					line=",  "$'\n\t\t\t'"  \"/usr/lib/$lib=$new_variant_dir_ts/lib/$lib\" "
					copy_stuff $indir/$lib_dir/peasoup_executable_dir $new_variant_dir/lib/$lib-peasoup_executable_dir $lib $new_variant_dir_ts/lib/$lib-peasoup_executable_dir 0

					# copy assurance evidence
					copy_assurance_evidence $indir/$lib_dir/peasoup_executable_dir/logs/assurance_case_evidence.log  $outdir/assurance/vs-${vs}_variant-${seq}_evidence.txt $lib 0 $config
				fi
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
					cp $new_variant_dir/bin/peasoup_executable_dir/ld-nol.so.ubuntu $outdir/global/
					cp $new_variant_dir/bin/peasoup_executable_dir/ld-nol.so.centos $outdir/global/
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
			if [[ $config == *"struct"* ]] ; then 
				echo $struct_set_size > $new_variant_dir/nolnoh_config
				echo $struct_set_no >> $new_variant_dir/nolnoh_config
				echo "	Struct noh/nol is enabled: $struct_set_no / $struct_set_size "
				if [[ $config == *"probNoh"* ]] || [[  $config == *"probNol"* ]] ; then
					echo
					echo "Cannot have structured xforms and probNoh or probNol.  Fatal error. "
					echo
					exit 1
				fi
			else
				# assume we are variant  #(rand 0-3)
				echo 4 > $new_variant_dir/nolnoh_config
			fi
	


			variant_name="variant_${vs}_${seq}"
			variant_config_contents="${variant_config_contents//<<EXEPATH>>/$new_variant_dir_ts/bin}"
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

	# copy libpthread_exit.so and fix_loader
	cp $CFAR_HOME/pthread_exit/libpthread_exit.so $outdir/global/
	cp $CFAR_HOME/non_overlapping_libraries/fix_loader.sh $outdir/global/


#	if [ $server = "APACHE" ]; then
#		ld_preload_var="/target_apps/global/libpthread_exit.so"
#	fi

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
	ld_preload_var="${ld_preload_var}:$extra_preloads"

	json_contents="${json_contents//<<ENV>>/\"LD_PRELOAD=$ld_preload_var\",<<ENV>>}"

	# if we are supposed to include checkpoint/restore lines in the file
	if [ "x"$use_includecr = "x--include-cr" ]; then
		echo "Including C/R support."
		# grab the appropriate contents for cp/rest and monitor settings.
		pre_checkpoint_cmd_contents=$(cat $PEASOUP_HOME/tools/cfar_configs/pre_checkpoint_cmd.json.template)
		cr_contents=$(cat $PEASOUP_HOME/tools/cfar_configs/cr_chunk.json.template)
		monitor_contents=$(cat $PEASOUP_HOME/tools/cfar_configs/monitor_chunk_with_cr.json.template)

		#echo setting monitor chunk to: $monitor_contents
		json_contents="${json_contents//<<CR>>/$cr_contents}"
		json_contents="${json_contents//<<MONITOR>>/$monitor_contents}"

		# if doing heap marshaling, use the precheckpoint cmd
		if [ "x"$use_diehard  = "x--diehard" ]; then
			json_contents="${json_contents//<<PRECHECKPOINTCMD>>/$pre_checkpoint_cmd_contents}"
		# else remove it
		else
			json_contents="${json_contents//<<PRECHECKPOINTCMD>>,/}"
			json_contents="${json_contents//<<PRECHECKPOINTCMD>>/}"
		fi
	else
		echo "Not doing C/R support."
		# grab the appropriate contents monitor settings and remove c/r marker
		monitor_contents=$(cat $PEASOUP_HOME/tools/cfar_configs/monitor_chunk.json.template)
		#echo setting monitor chunk to: $monitor_contents
		json_contents="${json_contents//<<CR>>/}"
		json_contents="${json_contents//<<MONITOR>>/$monitor_contents}"
	fi

	# substittue in the right settings for the monitor settings.

	# remove variant_config placeholders
	json_contents="${json_contents//,<<VARIANT_CONFIG>>/}"
	json_contents="${json_contents//,<<VARIANT_LIST>>/}"
	json_contents="${json_contents//,<<VARIANT_SETS>>/}"
	json_contents="${json_contents//,<<ENV>>/}"
	json_contents="${json_contents//<<ENV>>/}"
	json_contents="${json_contents//,<<LIBS>>/}"
	json_contents="${json_contents//<<LIBS>>/}"
	json_contents="${json_contents//,<<LDLIB>>/}"
	json_contents="${json_contents//<<LDLIB>>/}"
	json_contents="${json_contents//<<SERVER>>/$server}"
	json_contents="${json_contents//<<ARGS>>/$args}"
	json_contents="${json_contents//, <<CODE_MAP>>/}"
	json_contents="${json_contents//<<CODE_MAP>>/}"
	json_contents="${json_contents//, <<SCOOP_MAP>>/}"
	json_contents="${json_contents//<<SCOOP_MAP>>/}"
	json_contents="${json_contents//, <<P1_MAP>>/}"
	json_contents="${json_contents//<<P1_MAP>>/}"
	if [[ -z "$class" ]]; then
		# remove class field
		json_contents="${json_contents//<<CLASS>>,/}"
	else
		# put proper class in place.
		json_contents="${json_contents//<<CLASS>>/\"class\" : \"$class\"}"
	fi

	echo "$json_contents" > $json.ugly
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
