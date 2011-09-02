#!/bin/bash

#types="char short int long unsigned+char unsigned+short unsigned+int unsigned+long";
types="int char"

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

        # create the program.
        cat mul.ctmpl                       | \
                sed "s/#FUNCTION_NAME#/$function_name/g" | \
                sed "s/#FORMAT_SPECIFIER#/$format_specifier/g"     | \
                sed "s/#TYPE1#/$real_type1/g"     | \
                sed "s/#TYPE2#/$real_type2/g"     \
                        > $progname.c

	gcc -w $progname.c -o $progname.orig.exe
	
	$PEASOUP_HOME/tools/ps_analyze.sh --step ilr=off --step p1transform=off $progname.orig.exe $progname.protected.exe
}

for type1 in $types
do
	for type2 in $types
	do
		progname_c=mul.$type1.$type2
		# actually create the .c program
		create_prog $progname_c "$type1" "$type2" static_lib
	done
done
