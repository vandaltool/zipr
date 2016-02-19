#!/bin/sh

#
# The list of all benchmarks you'd like to test.  You can put a benchmark in the list multiple times if you'd like to
# test it more than once (say with different inputs).
#
# moved closer to the loop so we can see it
# benchnames="foo sample"
# backends="strata zipr"


#
# list of templates to generate tests with
#
#templates="stratafier.shtmpl stripped.shtmpl"


#
# create_test - this function create a test.  See parameter description immediately below.
#
create_test()
{
	#
	# name parameters
	#
	my_benchname=$1		# the name of the benchmark to invoke.
	my_backend=$2
	my_startfile=$3
	my_opt=$4
	my_pic=$5
	my_fp=$6
	my_debug=$7
	my_arch=$8
	my_template_name=$9	# the template file to use
	my_template_file=$10		# the template file to use

	# basename for the test script name
	disp_template=`basename $my_template_name .shtmpl`

	# default display values 
	disp_start=${my_startfile}
	disp_opt=${my_opt}
	disp_pic=${my_pic}
	disp_fp=${my_fp}
	disp_debug=${my_debug}

	# these might be changed to empty string
	actual_start=${my_startfile}
	actual_opt=${my_opt}
	actual_pic=${my_pic}
	actual_fp=${my_fp}
	actual_debug=${my_debug}
	
	# adjust for empty values
	if [ "${my_startfile}" = "none" ]; then 
		disp_start="no_startfile"; 
		actual_start=" "
	fi

	if [ "${my_opt}" = "none" ]; then 
		disp_opt="no_optflag"; 
		actual_opt=" "; 
	fi

	if [ "${my_pic}" = "none" ]; then 
		disp_pic="no_pic"; 
		actual_pic=" "
	fi

	if [ "${my_fp}" = "none" ]; then 
		disp_fp="no_omit-fp"; 
		actual_fp=" "; 
	fi

	if [ "${my_debug}" = "none" ]; then 
		disp_debug="no_debug"; 
		actual_debug=" "; 
	fi

	if [ X"${my_arch}" = X"-m32" ]; then
		actual_arch=x86_32
	else
		actual_arch=x86_64
	fi

	#
	# setup output filenames
	#
	scriptname=$my_benchname.${my_backend}.${disp_start}.${disp_opt}.${disp_pic}.${disp_fp}.${disp_debug}.${my_arch}.sh


	# build compiler flags list
	compflags="${actual_start} ${actual_opt} ${actual_pic} ${actual_fp} ${actual_debug} ${my_arch}"

	# Debug/echo so the user knows what's going down.
	echo benchname=$1, compflags=\"${compflags}\" scriptname=$scriptname, outname=$my_outname

	# 
	# actually create the script.
	#	
	cat $my_template_file 				| \
		sed "s/@BENCHNAME@/$my_benchname/g" 	| \
		sed "s/@BACKEND@/${my_backend}/g" 	| \
		sed "s/@ARCH@/${actual_arch}/g" 	| \
		sed "s/@BENCHFLAGS@/$real_benchflag/g"	| \
		sed "s/@COMPFLAGS@/${compflags}/g" 	\
			> ${my_benchname}_tests/$scriptname	

	#
	# record these files to be added 
	# 
	added_files="${my_benchname}/${scriptname} $added_files"
}


# loop over the variables
# run a separate loop for foo.cpp test program and the sample.cpp test program

# templates will follow the benchname

# benchnames="foo sample"
backends="strata zipr"
startfile="none -nostartfiles"
opts="none -O -O2 -O3 -Os"
pic="none -fPIC"
fp="none -fomit-frame-pointer"
debug="none -g"
archs="-m32 -m64"


# foo loop
mkdir -p foo_tests

template=template.foo.shtmpl
# foo test doesn't use startfiles flag so the loop will be different
for b in $backends
do
#   for s in $startfile
#   do
      for o in $opts
      do
         for p in $pic # compiler flags
         do
            for f in $fp
	    do
	       for d in $debug
   	       do
		  for a in $archs
		  do 
			# actually create the test
		
			create_test foo $b none $o $p $f $d $a $template $template
		  done	# arch
	       done	# debug
            done	# fp
         done 	# pic
      done 	# opts
#   done 	# startfile
done	# backends

mkdir -p sample_tests
# sample loop
template=template.sample.shtmpl
for b in $backends
do
   for s in $startfile
   do
      for o in $opts
      do
         for p in $pic # compiler flags
         do
            for f in $fp
	    do
	       for d in $debug
	       do
		  for a in $archs
		  do
			# actually create the test
			create_test sample $b $s $o $p $f $d $a $template $template
		  done	# arch
	       done	# debug
            done	# fp
         done 	# pic
      done 	# opts
   done 	# startfile
done	# backends


#
# svn add to the repo, but wait for user to verify/commit
#
echo "I've created these files.  Please verify them and commit as you see fit."
echo $added_files
#svn add $added_files
