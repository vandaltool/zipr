#!/bin/bash

types="char short int long unsigned+char unsigned+short unsigned+int unsigned+long";
#types="char short"
#sharedlib_gccflags="-O -O2 -fomit-frame-pointer"
sharedlib_gccflags="-O -O2 -fomit-frame-pointer"

#types="int char"

#
# The flags passed to the compiler
#
# note use of + to denote multiple parameters together.
# (but you can't use + as an argument to gcc.)
#

create_test()
{
	my_benchname=$1
	my_benchflag=$2
	my_gccflag=$3
	my_libtype=$4

        #
        # demangle the internal representation of the benchflags and gcc flags
        # substitute a + with a space
        #
        real_benchflag=`echo $my_benchflag | sed "s/+/ /g"`
        real_gccflag=`echo $my_gccflag | sed "s/+/ /g"`

        #
        # mangle the benchflags and gccflags for filename display. 
        #
        disp_benchflag=$my_benchflag
        disp_gccflag=$my_gccflag

        #
        # if the name is empty, reset to a nice string for naming.  Note:  This means you should not use "empty" as a parameter.
        #
        if [ $my_benchflag"X" = "X" ]; then disp_benchflag="nobenchflag"; fi;
        if [ $my_gccflag"X"   = "X" ]; then disp_gccflag="nogccflag";   fi;

	scriptname=$my_benchname.$disp_gccflag.$disp_benchflag.test.sh

        #
        # demangle the internal representation of the benchflags and gcc flags
        # substitute a + with a space
        #
        real_gccflag=`echo $my_gccflag | sed "s/+/ /g"`

        cat mul.shtmpl                       | \
                sed "s/@BENCHNAME@/$my_benchname/g"     | \
                sed "s/@LIBTYPE@/$my_libtype/g"         | \
                sed "s/@COMPFLAGS@/$real_gccflag/g"     \
                        > $scriptname

	added_files="$scriptname $added_files"
}

create_prog()
{
	progname=$1
	my_type1=$2
	my_type2=$3

	# substitute space for +
        real_type1=`echo $my_type1 | sed "s/+/ /g"`
        real_type2=`echo $my_type2 | sed "s/+/ /g"`

	# substitute _ for + to get a valid C function name
        name_type1=`echo $my_type1 | sed "s/+/_/g"`
        name_type2=`echo $my_type2 | sed "s/+/_/g"`
        function_name="test_${name_type1}_${name_type2}"

        case $my_type1 in
	"char") format_specifier="%c" ;;
	"short") format_specifier="%hd" ;;
	"int") format_specifier="%d" ;;
	"long") format_specifier="%ld" ;;
	"unsigned+char") format_specifier="%uc" ;;
	"unsigned+short") format_specifier="%hu" ;;
	"unsigned+int") format_specifier="%u" ;;
	"unsigned+long") format_specifier="%ul" ;;
	esac

        # create the source .c program.
        cat mul.ctmpl                       | \
                sed "s/#FUNCTION_NAME#/$function_name/g" | \
                sed "s/#FORMAT_SPECIFIER#/$format_specifier/g"     | \
                sed "s/#TYPE1#/$real_type1/g"     | \
                sed "s/#TYPE2#/$real_type2/g"     \
                        > $progname.c

	for gccflag in $sharedlib_gccflags
	do
		create_test $progname "" "$gccflag" shared_lib
	done

}

for type1 in $types
do
	for type2 in $types
	do
		progname_c=mul.$type1.$type2
		# actually create the .c program
		create_prog $progname_c "$type1" "$type2" 

	done
done

chmod +x *.sh

echo "would have added: $added_files"
