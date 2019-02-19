#!/bin/sh
# This script depends on having the following environment variables defined
# STRATA - The path to the strata installation
# An example of these environment variables and their settings are listed in
# the sample file: $STRATA/security_startup_rc


if [ "$PEASOUP_HOME"X = X ]; then echo Please set PEASOUP_HOME; exit 1; fi
if [ ! -f  $PEASOUP_HOME/tools/getsyms.sh ]; then echo PEASOUP_HOME is set poorly, please fix.; exit 1; fi
if [ "$SMPSA_HOME"X = X ]; then echo Please set SMPSA_HOME; exit 1; fi
if [ ! -f  $SMPSA_HOME/SMP-analyze.sh ]; then echo SMPSA_HOME is set poorly, please fix.; exit 1; fi


output=$1
stratafied_exe=$2
orig_exe=$3
annot_file=$4

sh $PEASOUP_HOME/tools/getsyms.sh $orig_exe
mv $orig_exe.syms $orig_exe.readelf

echo "#!/bin/sh" > $output
echo "PATH=$PATH:. STRATA_ANNOT_FILE=$annot_file STRATA_SYM_FILE=$orig_exe.readelf $stratafied_exe \$*" >> $output
chmod 755 $output


