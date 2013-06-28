#!/bin/bash

#
# Description: 
#    Extracts and computes set of string patterns given binary/library as input
#    The output string patterns are stored in a file in reverse-length order
#
# Usage:
#    generate_string_signatures.sh <filename> [<signatures> [<stringLogFile>]]
#
#    $1       'input file
#    $2       'output file 
#    $3       'input string log file 
#
# Output:
#    The file <$1.sigs> will contain a set of string patterns, one per line,
#    in reverse-length sorted order
#
# Intended Usage:
#    To enable a cheap way of specifying tainted data in a binary program
#    The key observation is that trusted/native strings from which SQL queries are constructed
#    will be present as strings in the binary. Such strings can therefore be marked as trusted, 
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

defaultSigs=$PEASOUP_HOME/tools/signatures/sqlite.sigs

inputFile=$1
if [ -z $2 ];then
	finalSigFile=$1.sigs
else
	finalSigFile=$2
fi

stringLogFile=$3

tmpFile=$1.$$.tmp
tmpFile2=$1.$$.2.tmp
tmpFile3=$1.$$.3.tmp
tmpFile4=$1.$$.4.tmp
tmpFile5=$1.$$.5.tmp
tmpSymbols=$1.$$.sym.tmp

# setup/cleanup
rm $finalSigFile 2>/dev/null
rm $tmpFile $tmpFile2 $tmpFile3 $tmpFile4 $tmpFile5 $tmpSymbols 2>/dev/null
touch $tmpFile2 $tmpFile3 $tmpFile4 $tmpFile5 $finalSigFile

# get strings & symbols
if [ -z $stringLogFile ]; then
	# get strings from ELF file
	strings -n 2 $inputFile | sort  | uniq  > $tmpFile2            
else
    # get string from smart string extractor (used by ps_analyze.sh)
	grep -i "found string:" $stringLogFile | sed "s/Found string: \"//" | sed "s/\" at.*$//" > $tmpFile2
fi

cat $defaultSigs >> $tmpFile2                                          # add signatures from sqlite itself
sort  $tmpFile2 | uniq  > $tmpFile                                 

nm -a $inputFile | grep -v " U " | grep -v " w " | sort | uniq | cut -d' ' -f3 > $tmpSymbols # get symbol names

#
# break up strings with potential format specifiers
# and add them to the list of patterns
#

# @todo: this is a hack until we use regular expressions

#
# look for %s, %d and other format specifiers
#
# @todo make this a function and call a whole bunch of time
# until no more delimeters are found
#
# @todo don't do it here, instead use regex in library code
# to get patterns
#     SELECT * from table where userid='%s'     --> %s should become a regex pattern that matches all valid strings
#     SELECT * from table where userid='%d'     --> %s should become a regex pattern that matches all valid numbers
#

# grab lhs and rhs
grep "%[-]*[0-9]*s" $tmpFile | sed "s/%[-]*[0-9]*s/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[-]*[0-9]*s" $tmpFile | sed "s/%[-]*[0-9]*s/#/" | cut -d'#' -f2 >> $tmpFile3
grep "%[0-9]*[l,ll]*d" $tmpFile | sed "s/%[0-9]*[l,ll]*d/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*[l,ll]*d" $tmpFile | sed "s/%[0-9]*[l,ll]*d/#/" | cut -d'#' -f2 >> $tmpFile3
grep "%[0-9,.]*f" $tmpFile | sed "s/%[0-9,.]*f/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9,.]*f" $tmpFile | sed "s/%[0-9,.]*f/#/" | cut -d'#' -f2 >> $tmpFile3
cat $tmpFile3 >> $tmpFile2

# do it one more time to split the previous rhs into 2 parts
grep "%[0-9]*s" $tmpFile3 | sed "s/%[0-9]*s/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*s" $tmpFile3 | sed "s/%[0-9]*s/#/" | cut -d'#' -f2 >> $tmpFile4
grep "%[0-9]*[l,ll]*d" $tmpFile3 | sed "s/%[0-9]*[l,ll]*d/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*[l,ll]*d" $tmpFile3 | sed "s/%[0-9]*[l,ll]*d/#/" | cut -d'#' -f2 >> $tmpFile4
grep "%[0-9,.]*f" $tmpFile3 | sed "s/%[0-9,.]*f/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9,.]*f" $tmpFile3 | sed "s/%[0-9,.]*s/#/" | cut -d'#' -f2 >> $tmpFile4
cat $tmpFile4 >> $tmpFile2

# and yet one more time for good measure
grep "%[0-9]*s" $tmpFile4 | sed "s/%[0-9]*s/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*s" $tmpFile4 | sed "s/%[0-9]*s/#/" | cut -d'#' -f2 >> $tmpFile5
grep "%[0-9]*[l,ll]*d" $tmpFile4 | sed "s/%[0-9]*[l,ll]*d/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9]*[l,ll]*d" $tmpFile4 | sed "s/%[0-9]*[l,ll]*d/#/" | cut -d'#' -f2 >> $tmpFile5
grep "%[0-9,.]*f" $tmpFile4 | sed "s/%[0-9,.]*f/#/" | cut -d'#' -f1 >> $tmpFile2
grep "%[0-9,.]*f" $tmpFile4 | sed "s/%[0-9,.]*s/#/" | cut -d'#' -f2 >> $tmpFile5
cat $tmpFile5 >> $tmpFile2

##
## @todo: if as a result of breaking up format strings, we get for example
##        a SQL keyword, e.g., table, drop, we shouldn't add it to the list of
##        string patterns
##
## another policy in general could be to not have patterns specifiers that are
## too small in length
##

# get rid of duplicate entries
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
cut -f2- -d' ' $tmpFile | uniq  > $finalSigFile

# et voila, output file $finalSigFile contains the final reverse sorted set of patterns

# now cleanup
rm $tmpFile $tmpFile1 $tmpFile2 $tmpFile3 $tmpFile4 $tmpFile5 $tmpSymbols

exit 0
