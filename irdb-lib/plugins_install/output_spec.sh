#!/bin/bash -x


varid="$1"
fileopt="$2"
file="$3"

usage()
{
	echo "

Usage:

	output_spec.sh <varid> --file 'file1.attr file2.attr ...'

"
}

main()
{

	if [ "X$fileopt" != "X--file" ]; then
		usage
		exit 1
	fi

	echo "# ATTRIBUTE backend=$backend" $i >  $file
	echo "# ATTRIBUTE peasoup_dir=$newdir" $i >>  $file
	echo "# ATTRIBUTE output_file=$stratafied_exe" $i >>  $file
	echo "# ATTRIBUTE pwd=$PWD" $i >>  $file


}

main "$@"
