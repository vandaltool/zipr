#!/bin/bash

#
# Description: 
#    Extracts and computes set of string patterns given binary/library as input
#    The output string patterns are stored in a file in reverse-length order
#
# Usage:
#    generate_signatures <filename>
#
#    $1       'input file
#
# Output:
#    The file <$1.sigs> will contain a set of string patterns, one per line,
#    in reverse-length sorted order
#
# Intended Usage:
#    To enable a cheap way of specifying tainted data in a binary program
#    The key observation is that trusted/native strings from which SQL queries are constructed
#    will be present as srings in the binary. Such strings can therefore be marked as trusted, 
#    or non-tainted. Any portion of a string passed to a critical function 
#    that is not found in the binary is probably external, and therefore
#    should be marked as potentially tainted.
#
#    This quick & dirty way of specifying taint markings should work for a variety of interpreters,
#    including SQL, LDAP, XPath, and Shells.
#
# Blame/Credit: Original idea by Jason Hiser. 
# Author: Anh
#
# Future extensions:
#    - Combine with profiling info
#    - Protection for shell/sql/ldap/xml/... injection attacks
#
# Features:
#    - Filters out any found symbol names from output file
#    - Uses simple heuristics to break up potential format strings
#        for example, "hello %s, how are you", will result in the following 3 patterns in the output file:
#              (1) hello %s, how are you
#              (2) hello 
#              (3) , how are you
#        nb: line (2) has a trailing space
# 
# Log
# ----------------------------------------------------------------------------------
# 20120713 Anh - decent working prototype, simple splitting format string specifiers
#

inputFile=$1
finalSigFile=$1.sigs
tmpFile=$1.$$.tmp
tmpFile2=$1.$$.2.tmp
tmpFile3=$1.$$.3.tmp
tmpSymbols=$1.$$.sym.tmp

# setup/cleanup
rm $finalSigFile 2>/dev/null
rm $tmpFile $tmpFile2 $tmpFile3 $tmpSymbols 2>/dev/null
touch $tmpFile2 $tmpFile3 $finalSigFile

# get strings & symbols
strings $inputFile | sort -f | uniq -i > $tmpFile                                            # get strings
nm -a $inputFile | grep -v " U " | grep -v " w " | sort | uniq | cut -d' ' -f3 > $tmpSymbols # get symbol names

#
# break up strings with potential format specifiers
# and add them to the list of patterns
#

# @todo: verify these patterns

#
# look for %s, %d and other format specifiers
# we currently only handle 2 delimiters in string
#
# @todo make this a function and call a whole bunch of time
# until no more delimeters are found
#

# grab lhs and rhs
grep "%[0-9]*s" $tmpFile | sed "s/%[0-9]*s/#/g" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*s" $tmpFile | sed "s/%[0-9]*s/#/g" | cut -d'#' -f2 >> $tmpFile3
grep "%[0-9]*[l,ll]*d" $tmpFile | sed "s/%[0-9]*[l,ll]*d/#/g" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*[l,ll]*d" $tmpFile | sed "s/%[0-9]*[l,ll]*d/#/g" | cut -d'#' -f2 >> $tmpFile3
grep "%[0-9,.]*f" $tmpFile | sed "s/%[0-9,.]*f/#/g" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9,.]*f" $tmpFile | sed "s/%[0-9,.]*f/#/g" | cut -d'#' -f2 >> $tmpFile3

# do it one more time to split the previous rhs into 2 parts
grep "%[0-9]*s" $tmpFile3 | sed "s/%[0-9]*s/#/g" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*s" $tmpFile3 | sed "s/%[0-9]*s/#/g" | cut -d'#' -f2 >> $tmpFile3
grep "%[0-9]*[l,ll]*d" $tmpFile3 | sed "s/%[0-9]*[l,ll]*d/#/g" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*[l,ll]*d" $tmpFile3 | sed "s/%[0-9]*[l,ll]*d/#/g" | cut -d'#' -f2 >> $tmpFile3
grep "%[0-9,.]*f" $tmpFile3 | sed "s/%[0-9,.]*f/#/g" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9,.]*f" $tmpFile3 | sed "s/%[0-9,.]*s/#/g" | cut -d'#' -f2 >> $tmpFile3

cat $tmpFile3 >> $tmpFile2

# add to signature list
sort $tmpFile2 | uniq >> $tmpFile

#
# iterate through each line, prepend the length 
# we'll then use the length to reverse sort
# as the final output file must be in reverse order (sorted by length)
#
while read signature;
do
#  echo "${signature}" > $tmpFile2
#  length=$(wc -c $tmpFile2 | cut -f1 -d' ')
#  echo "TEST: [${signature}]: cmd: [grep -F -x \"$signature\" $tmpSymbols]"
  grep -F -x -e "${signature}" $tmpSymbols >/dev/null 2>/dev/null
  if [ ! $? -eq 0 ];
  then
    echo "${#signature} ${signature}" >> $finalSigFile    # metacharacters can confuse ${#signature}, length will be wrong?
#    echo "$length ${signature}" >> $finalSigFile    # metacharacters can confuse ${#signature}, length will be wrong?
  fi

done < $tmpFile

# perform reverse numeric sort
sort -n -r $finalSigFile > $tmpFile

# get rid of the length field
cut -f2- -d' ' $tmpFile | uniq -i > $finalSigFile

exit 0
# et voila, output file $finalSigFile contains the final reverse sorted set of patterns

# now cleanup
rm $tmpFile $tmpFile1 $tmpFile2 $tmpSymbols
