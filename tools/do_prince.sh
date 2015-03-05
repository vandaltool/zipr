#!/bin/bash
#
#
# cinderella.spec
#   strlen
#   strdup
#   ...
#
#
# $PEASOUP_HOME/tools/do_prince.sh `pwd`/$TESTABLE $PEASOUP_HOME/tools/cinderella.spec functions.all.addresses

binary=$1
functions_to_test=$2
addresses=$3
malloc_addresses=$4

out=cinderella.inferences
touch $out

tmp=$functions_to_test.tmp.$$
tr -s '\r\n' ' ' < $functions_to_test | sed -e 's/ $/\n/' > $tmp
allfunctions=`cat $tmp`
rm $tmp

if [ -z $malloc_addresses ]; then
	for fn in $allfunctions
	do
		cmd="$PEASOUP_HOME/cinderella/prince.sh $binary $fn $addresses"
		echo "DO PRINCE: $cmd"
		$cmd | grep "^prince" | grep $fn | grep positive >> $out.positive
		$cmd | grep "^prince" | grep $fn | grep negative >> $out.negative
	done
	cp $out.positive $out
else
	# look for mallocs
	tmp=$malloc_addresses.tmp.$$
	tr -s '\r\n' ' ' < $malloc_addresses | sed -e 's/ $/\n/' > $tmp
	allmallocs=`cat $tmp`
	rm $tmp

	for malloc in $allmallocs
	do
		echo "DO PRINCE: assume malloc at $malloc"
		for fn in $allfunctions
		do
			echo "DO PRINCE: assume malloc at $malloc -- dynamic test function $fn"
			cmd="$PEASOUP_HOME/cinderella/prince.sh $binary $fn $addresses $malloc"
			$cmd | grep "^prince" | grep $fn | grep positive >> $out.allocator.positive
			$cmd | grep "^prince" | grep $fn | grep negative >> $out.allocator.negative
		done
	done
	cp $out.allocator.positive $out.allocator
fi

