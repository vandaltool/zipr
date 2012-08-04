#!/bin/sh 

#
# note:  no trailing slashes, as the comparison will fail.
#
safe_dir_list="/lib /lib/tls/i686/cmov /usr/lib"
#safe_dir_list="/lib /usr/lib"

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

