#!/bin/bash

cat a.irdb.fbspri|grep -e '->'|grep -v -e '-> 0x0'|egrep -v -e "^ff"|sed "s/->.*//" > pinned_addresses


