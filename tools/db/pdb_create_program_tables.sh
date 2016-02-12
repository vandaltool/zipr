#!/bin/sh  -x
#
# Copyright (c) 2014 - Zephyr Software LLC
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


#
# pdb_create_program_tables <atn> <ftn> <itn> <icfs> <icfs_map> <rtn> <typ> file
#


# remove any path name

atn=$1
ftn=$2
itn=$3
icfs=$4
icfs_map=$5
rtn=$6
typ=$7
dtn=$8
file=$9

echo Creating tables $atn, $ftn, $itn, $icfs, $icfs_map, $rtn, $typ, and $dtn.

DB_SCRIPT=$file
cat $PEASOUP_HOME/tools/db/pdb.createprogram.tbl |  \
                sed "s/#ATN#/$atn/g" | \
                sed "s/#FTN#/$ftn/g" | \
                sed "s/#ITN#/$itn/g" | \
                sed "s/#DTN#/$dtn/g" | \
                sed "s/#ICFS#/$icfs/g" | \
                sed "s/#ICFS_MAP#/$icfs_map/g" | \
                sed "s/#RTN#/$rtn/g" | \
                sed "s/#TYP#/$typ/g"  \
                > $DB_SCRIPT

