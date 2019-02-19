#!/bin/sh 

cat $1 |sed -e "s/#.*//g" -e "/^$/d" |grep -v " rl " > $2
cat $1 |sed -e "s/#.*//g" -e "/^$/d" |grep  " rl " >> $2


exit 0
