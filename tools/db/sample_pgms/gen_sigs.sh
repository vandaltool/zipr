#!/bin/sh

num=0
cd slash_usr_bin
rm -f *.strsig
for i in /usr/bin/*; do
	if [ -x $i ]; then 
		$PEASOUP_HOME/tools/make_prog_signature.sh $i `basename $i`.strsig
		num=`expr $num + 1`
	else
		echo skipping $i
	fi
done
cd ..




cd slash_bin
rm -f *.strsig
for i in /bin/*; do
	if [ -x $i ]; then 
		$PEASOUP_HOME/tools/make_prog_signature.sh $i `basename $i`.strsig
		num=`expr $num + 1`
	else
		echo skipping /bin/$i
	fi
done
cd ..







for i in dr1 dr2
do
	cd $i
	rm -f *.strsig
	for j in *
	do
		$PEASOUP_HOME/tools/make_prog_signature.sh $j $j.strsig
		num=`expr $num + 1`
	done
	cd ..
done

echo Found $num programs to signature.


