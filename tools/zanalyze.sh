#/bin/bash


usage()
{
	echo '
zanalyze.sh 

  # this scren
  --help

  # specify input.
  (
    (--inzar|-z) <input.zar> | 
    (-i|--infiles) "file1 file2 file3 ..." |
    (-m|--inmanifest) <input.json>
  ) 

 # specify output.
 (-o|--outfile| <output.zar,output.exe,... defaults to $input.protected.zar>)

 # specify how to protect the input.
 [(-p|—protection_engine) </path/to/pretection engine and options>
     e.g. cfar.sh|ps_analyze.sh|ps_analyze_cgc.sh|cfar_probP1_zipr.sh|cfar_probP1_strata.sh 
     defaults to "$P_H/tools/ps_analyze.sh --backend zipr">
 ] 

 # specify one-by-one or all-at-once protection (zipr/strata differences in protection engines)
 [(-x|--pe_mode) [(i|individual)|(m|multi) 
     defaults to "-x i"
 ] 

 # Specify a collection engine -- this is a "finishing" step after the protection engine has finished 
 # processing all the inputs.  Can be used to collect results into installers, tarballs, etc.
 (-c|—collection-engine) <engine and options, defaults to "$PEASOUP_HOME/tools/zipr_ce.sh>" 

 # specify a path for an output file containing a description of what happened -- for future expansion, ignored for now.
 [(-s|--output_spec) <filename.attr>]
'

}

check_options()
{


        # Note that we use `"$@"' to let each command-line parameter expand to a 
        # separate word. The quotes around `$@' are essential!
        # We need TEMP as the `eval set --' would nuke the return value of getopt.
        short_opts="z:i:m:o:p:x:c:s:h"
        long_opts="--long inzar:
                   --long infiles:
                   --long inmanifest:
                   --long outfile:
                   --long protection_engine:
                   --long pe_mode:
                   --long collection_engine
                   --long output_spec:
                   --long help
                "

        # solaris does not support long option names
        if [ `uname -s` = "SunOS" ]; then
                TEMP=`getopt $short_opts "$@"`
        else
                TEMP=`getopt -o $short_opts $long_opts -n 'zanalyze.sh' -- "$@"`
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
#			--inzar|-z)
#				echo "not impl'd"
#				exit 1
#			;;
#			--inmanifest|-m)
#				echo "not impl'd"
#				exit 1
#			;;
			--infiles|-i)
				infiles="$2"
				shift 2
			;;
			--outfile|-o)
				outfile="-o $2"
				shift 2
			;;
			--protection_engine|-p)
				protection_engine="$2"
				shift 2
			;;
			--pe_mode|-x)
				if [ "X$2" = "Xi" ]; then
					pe_mode="individual"
				elif [ "X$2" = "Xindividual" ]; then
					pe_mode="individual"
				elif [ "X$2" = "Xm" ]; then
					pe_mode="multi" 
				elif [ "X$2" = "multi" ]; then
					pe_mode="multi"
				else
					echo "--pe_mode $pe_mode not understood"
					exit 1
				fi
				shift 2
			;;
			--collection_engine|-c)
				collection_engine="$2"
				shift 2
			;;
#			--output_spec|-s)
#				output_spec="--output_spec $2"
#				shift 2
#			;;
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

}

check_environ_vars()
{

        while [ true ]; 
        do

                # done?
                if [ -z $1 ]; then
                        return;
                fi

                # create the $ENVNAME string in varg
                varg="\$$1"

                # find out the environment variable's setting
                eval val=$varg

                if [ -z $val ]; then echo Please set $1; exit 1; fi

                shift
        done

}


expand_zar()
{
	# do nothing yet
	# eventually expand and set infile to set of input files
	echo  -n
}

do_individual_protection()
{
	seq=0
	for i in $infiles
	do
		file=$PWD/manifest$seq.attr
		echo Attempting: $engine $i $i.protected $engine_options --step output_spec=on --step-option output_spec:"--file $file"
		$engine $i $i.protected $engine_options --step output_spec=on --step-option output_spec:"--file $file"
		engine_res="$?"

		if  [  $engine_res != 0 ]; then
			echo "Engine protection failed.  Aborting..."
			exit  $engine_res
		fi
		seq=$(expr $seq + 1)
		
		intermediate_attribute_files="$intermediate_attribute_files $file"
	done
}

do_multi_protection()
{
	echo "Multi protection not yet implemented."
	exit 1
}

invoke_collection_engine()
{
	echo "Attempting: $collection_engine --attrfiles '$intermediate_attribute_files' $outfile"
	$collection_engine --attrfiles "$intermediate_attribute_files" $outfile
}


parse_protection_engine()
{
	engine=$(echo $protection_engine | cut -d' ' -f1)
	engine_options=$(echo $protection_engine' ' | cut -d' ' -f2-)

}

main()
{

	check_environ_vars PEASOUP_HOME 

	outspec=""
        intermediate_attribute_files=""
	collection_engine="$PEASOUP_HOME/tools/zipr_ce.sh"
	protection_engine="$PEASOUP_HOME/tools/ps_analyze.sh --backend zipr"
	pe_mode="individual"
	outfile="-o output.zar"

	check_options "$@"

	parse_protection_engine

	expand_zar 

	if  [ "$pe_mode" = "individual" ]; then
		do_individual_protection
	elif  [ "$pe_mode" = "multi" ]; then
		do_multi_protection
	else
		echo "--pe_mode $pe_mode not understood"
		exit 1
	fi

	invoke_collection_engine


	echo Infile=$infiles

}


# execute the program
main "$@"
