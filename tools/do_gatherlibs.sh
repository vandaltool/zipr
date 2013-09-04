#!/bin/sh  -x

#
# note:  no trailing slashes, as the comparison will fail.
#
safe_dir_list="/lib /lib/tls/i686/cmov /usr/lib /lib/i686/cmov /lib/i386-linux-gnu \
	/usr/local/lib /usr/lib/i386-linux-gnu /lib/x86_64-linux-gnu /lib64"
#safe_dir_list="/lib /usr/lib"

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

for i in `$PEASOUP_HOME/tools/getlibs.sh a.ncexe`
do
	is_safe $i 
	if [ $? = 0 ]; then
		echo Copying $i.
		echo `basename $i` >> shared_libs
		cp $i shared_objects
	else
		echo "Skipping $i as it is detected as safe."
	fi
done

