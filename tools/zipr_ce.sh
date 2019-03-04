#!/bin/bash

usage()
{
echo "

Usage:
	zipr_ce.sh (--help|-h|--usage)
	zipr_ce.sh --(-a|--attrfiles) 'file1 file2 file3 ...' (-o|--output) <output.zar>
"
}

check_options()
{


        # Note that we use `"$@"' to let each command-line parameter expand to a 
        # separate word. The quotes around `$@' are essential!
        # We need TEMP as the `eval set --' would nuke the return value of getopt.
        short_opts="a:o:h"
        long_opts="--long attrfiles:
                   --long ouput:
                   --long help
		   --long usage
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
                        --help|-h|--usage)
                                usage;
                                exit 1
			;;
                        --outfile|-o)
                                outfile="$2"
				shift 2
			;;
                        --attrfiles|-a)
                                attrfiles="$2"
				shift 2
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

}

tarfiles()
{

	for attrfile in $attrfiles
	do
		exefile=$(cat $attrfile |grep "#ATTRIBUTE output_file="|sed "s/#ATTRIBUTE output_file=//")
		exefiles="$exefiles $exefile"
	done
	
	tar cf $outfile $exefiles
	
	
}

main()
{
	check_options "$@"
	tarfiles

	echo Complete.
}


main "$@"



