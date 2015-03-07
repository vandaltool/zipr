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

variant_id=$1
binary=$2                       # binary to test
libcfunctions_filepath=$3       # file with list of libc functions to look for
allfunctions_filepath=$4        # file with function names from binary to test
malloc_addresses=$5

PRINCE_SCRIPT=$SECURITY_TRANSFORMS_HOME/tools/prince/prince.sh

out=cinderella.inferences
touch $out

echo "file with list of libc functions: $libcfunctions_filepath"
tmp=$libcfunctions_filepath.tmp.$$
tr -s '\r\n' ' ' < $libcfunctions_filepath | sed -e 's/ $/\n/' > $tmp
alllibcfunctions=`cat $tmp`
rm $tmp

if [ -z $malloc_addresses ]; then
	for fn in $alllibcfunctions
	do
		cmd="$PRINCE_SCRIPT $variant_id $binary $fn $allfunctions_filepath"
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
		for fn in $alllibcfunctions
		do
			echo "DO PRINCE: assume malloc at $malloc -- dynamic test function $fn"
			cmd="$PRINCE_SCRIPT $variant_id $binary $fn $allfunctions_filepath $malloc"
			$cmd | grep "^prince" | grep $fn | grep positive >> $out.allocator.positive
			$cmd | grep "^prince" | grep $fn | grep negative >> $out.allocator.negative
		done
	done
	cp $out.allocator.positive $out.allocator
fi

