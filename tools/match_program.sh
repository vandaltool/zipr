#!/bin/sh 

abs () {
	# Check for numeric input
	if expr $1 + 0 2>/dev/null 1>&2 ; then
		# Is the number negative?
		if [ $1 -lt 0 ] ; then
			echo `expr 0 - $1`
		else
			echo $1
		fi
		return 0 # OK
	else
		return 1 # Not a number
	fi
}



$PEASOUP_HOME/tools/make_prog_signature.sh a.ncexe a.ncexe.strsig

lines1=`cat a.ncexe.strsig| wc -l `
best_score=101
second_best=101
best_file=""

#echo score diffs lines1 lines2 filename
for i in `find $PEASOUP_HOME/tools/db/ -print|egrep ".strsig$"`
do
	num_diffs=`diff -U 0 $i a.ncexe.strsig |grep -v "^@" |wc -l`
	lines2=`cat $i | wc -l `
	
	best=`expr $num_diffs \* 100 / \( $lines1 + $lines2 \) `

	if [ $best -lt 30 ]; then 
		echo $best $num_diffs $lines1 $lines2 $i
	fi

	if [ $best -le $best_score ]; then
		#echo found new best $i
		second_best=$best_score
		second_file=$best_file
		best_score=$best
		best_file=$i
	fi
done

# check for a good match
if [ $best_score -lt 30 ]; then
	echo "Program is a version of '$(basename $best_file .strsig)'"
else
	echo Could not determine program.
fi

