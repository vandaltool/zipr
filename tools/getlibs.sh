#!/bin/sh

remove_dups()
{
	rm -f /tmp/dups.$$
	for j in $*; do
		echo $j >> /tmp/dups.$$
	done
	cat /tmp/dups.$$|sort |uniq
	rm -f /tmp/dups.$$
}

resolve(){

	file=`ldconfig -p |grep "$1 ("|head  -1|sed "s/.*=>//"`
	if [ ! -z "$file" -a -e $file ]; then
		echo -n
		echo $file
	else
		echo $1
	fi

}


found=1

alreadyFound=$1

while [ $found = 1 ] ; do

	found=0

	list="$alreadyFound"

	for i in $alreadyFound; do
		file=`resolve $i`
		newlist=`readelf -d  $file |grep NEEDED|sed -e "s/.*\[//" -e "s/\]//"`
		list=`remove_dups $list $newlist`
	done

	if [ "$alreadyFound" != "$list" ]; then
		alreadyFound="$list"
		found=1
	fi
done

for i in $alreadyFound ; do
	if [ ! $1 = $i ]; then 
		echo `resolve $i` 
	fi
done

