#!/bin/bash   


#
# Default safe dir list.  Use options to override.
# note:  no trailing slashes, as the comparison will fail.
#
safe_dir_list="	/lib /lib/tls/i686/cmov 			\
		/usr/lib /lib/i686/cmov 			\
		/lib/i386-linux-gnu 				\
		/usr/local/lib					\ 
		/usr/lib/i386-linux-gnu 			\ 
		/lib/x86_64-linux-gnu				\
		/usr/lib/x86_64-linux-gnu /lib32		\
		/lib/i386-linux-gnu/i686/cmov			\
		/usr/lib64					\
		/lib64						\
	"

exe=$0

these=""


# parse arguments
while [[ $# > 0 ]]
do
	key="$1"

	case "$key" in
		--main_exe_only)
			echo "Protecting no shared libraries, doing main exe only."
			# skip 
			exit 0 # no need to run the program
			;;
		--all)
			echo "Protecting ALL shared libraries."
			# nothing is safe.
			safe_dir_list=""
			;;
		--safe)
			# default is to use safe list.
			echo "Using default safe list:"
			echo "$safe_dir_list"
			
			;;
		--protectthese)
			if [[ $# < 1 ]]; then
				echo "--protectthese needs an option"
				exit 1 # reported error
			fi
			shift

			while  [[ "$#" > 0 ]] && [[ "$1" != "-*" ]]; 
			do
				these="$these $1"
				echo "Protecting file: $1"
				shift
			done
			;;
		--safelist)
			if [[ $# < 1 ]]; then
				echo "--safelist needs an option"
				exit 1 # reported error
			fi
			shift
			safe_dir_list="$1"
			echo "Using custom safe list:"
			echo "$safe_dir_list"
			;;
		*|--usage)
			echo "Usage: "
			echo "  $exe { --main_exe_only | --all | --safe | --usage | --safelist 'path1 path2 ...' | --protectthese 'lib1.so lib2.so ...'}"
			exit 1 # report error as we didnt parse all options, etc.
			;;
	esac
	shift
done



# Add all library directories under /opt/stonesoup/dependencies if present
if [ -d "/opt/stonesoup/dependencies" ]; then
	stonesoup_dir_list=$( ( for d in `find /opt/stonesoup/dependencies/* -name 'lib*.so'`; do dirname $(realpath $d); done ) | sort | uniq | tr "\\n" " " )
	safe_dir_list="$safe_dir_list $stonesoup_dir_list"
fi

is_safe()
{
	for j in $safe_dir_list; do
		if [ $j = `dirname $1` ]; then
			return 1
		fi
	done
	return 0
}



mkdir shared_objects
rm -f shared_libs
touch shared_libs


if [ "X$these" != "X" ]; then
	for i in $these
	do
		if [ ! -f $i ]; then
			echo "Missing library file $i" 
			echo "Missing library file $i" > warning.txt
			exit 255
		fi
		cp $i shared_objects
		echo `basename $i` >> shared_libs
	done
	# after copying all libraries, we're done.  we were told explicitly what to protect
	exit 0
fi

libs=`$PEASOUP_HOME/tools/getlibs.sh a.ncexe`

if [ $? -ne 0 ]; then
	echo Failed to gather all libraries.  
	exit 1
fi

for i in  $libs
do
	is_safe $i 
	if [ $? = 0 ]; then
		echo Copying $i.
		echo `basename $i` >> shared_libs
		if [ ! -f $i ]; then
			echo Missing library fille $i
			exit 255
		fi
		cp $i shared_objects
	else
		echo "Skipping $i as it is detected as safe."
	fi
done

