#!/bin/sh 
#
# Copyright (c) 2015 - Zephyr Software LLC
#
# This file may be used and modified for non-commercial purposes as long as
# all copyright, permission, and nonwarranty notices are preserved.
# Redistribution is prohibited without prior written consent from Zephyr
# Software.
#
# Please contact the authors for restrictions applying to commercial use.
#
# THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
# Author: Zephyr Software
# e-mail: jwd@zephyr-software.com
# URL   : http://www.zephyr-software.com/
#

psql -U $Z_PGUSER -h $Z_PGHOST -p $Z_PGPORT -d $Z_PGDATABASE -f $PEASOUP_HOME/tools/db/zest/zest.drop.tbl
