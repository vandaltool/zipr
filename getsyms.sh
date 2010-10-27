#!/bin/sh
readelf -s $1 |grep -e FUNC -e OBJECT|cut -d: -f2|$NICECAP_HOME/chopzero|sort --key=1,9 --key=13,21|cut -c 2-31,44-200|uniq -w 14 > $1.syms

