#!/bin/sh


# uva's svn version.  this sshould be "old" with uva's edits.
update_dir=/home/jdh8d/64bit_port/uvadev.peasoup/security_transforms/beaengine/

# output location -- should be a copy of uva's svn version.
new_dir=/home/jdh8d/64bit_port/uvadev.peasoup/security_transforms/beaengine2/

# update_dir as a sed pattern.
sed_update_dir=".home.jdh8d.64bit_port.uvadev.peasoup.security_transforms.beaengine."

#
# path to the "old" version of bea engine.
#
old=beaengine166

#
# path to the "new" version of bea engine.
#
new=beaengine175

for i in  `find $update_dir -print |egrep -v \.svn`; 
do
	bn=`echo $i |sed "s/$sed_update_dir//"`

	if [ -f $update_dir/$bn  -a -f $old/$bn -a -f $new/$bn ]; then
		diff $old/$bn $new/$bn > /dev/null
	
		if [ $? != 0 ]; then 
			echo Found $bn with possible patches
			# this command auto-patches the difs between old/new into update_dir 
			#diff3 -m $i $old/$bn $new/$bn  > $new_dir/$bn
		fi
	elif [ -f $old/$bn -a ! -f $new/$bn ]; then
		echo $bn is removed in $new
	elif [ ! -f $old/$bn -a -f $new/$bn ]; then
		echo $bn is added in $new
	elif [ -d  $i ]; then
		# empty, this is a dir
		echo -n
	else 
		
		echo skipping $bn 
	fi
done
