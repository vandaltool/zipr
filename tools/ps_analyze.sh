#!/bin/sh 
# This script depends on having the following environment variables defined
# STRATA - The path to the strata installation
# An example of these environment variables and their settings are listed in
# the sample file: $STRATA/security_startup_rc
#
# Usage:
#     peasoup_analyze.sh <original_binary> [ <new_binary> ] 
#
# Version 1 - prepares binary for PC confinement
# Version 2 - runs Grace
# Version 3 - runs p1 transform




log()
{
	if [ ! -z "$VERBOSE" ]; then
		cat $1
	fi
}

if [ ! -z "$VERBOSE" ]; then
	set -x
fi

if [ "$PEASOUP_HOME"X = X ]; then echo Please set PEASOUP_HOME; exit 1; fi
if [ ! -f  $PEASOUP_HOME/tools/getsyms.sh ]; then echo PEASOUP_HOME is set poorly, please fix.; exit 1; fi
if [ "$SMPSA_HOME"X = X ]; then echo Please set SMPSA_HOME; exit 1; fi
if [ ! -f  $SMPSA_HOME/SMP-analyze.sh ]; then echo SMPSA_HOME is set poorly, please fix.; exit 1; fi
if [ "$STRATA_HOME"X = X ]; then echo Please set STRATA_HOME; exit 1; fi
if [ ! -f  $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh ]; then echo STRATA_HOME is set poorly, please fix.; exit 1; fi
if [ "$SECURITY_TRANSFORMS_HOME"X = X ]; then echo Please set SECURITY_TRANSFORMS; exit 1; fi

if [ -z $2 ]; then
  echo "Usage: $0 <original_binary> <new_binary>"
  exit 1
fi

orig_exe=$1
newname=a

if [ -z $2 ]; then
	stratafied_exe=$orig_exe
else
	stratafied_exe=$2
fi

date
echo "Original program: $orig_exe   New program: $stratafied_exe"

name=`basename $orig_exe`
newdir=peasoup_executable_directory.$name.$$

mkdir $newdir
cp $orig_exe $newdir/$newname.ncexe
rm $stratafied_exe
cd $newdir


echo -n Creating stratafied executable...
sh $STRATA_HOME/tools/pc_confinement/stratafy_with_pc_confine.sh $newname.ncexe $newname.stratafied > pc_confinement.out  2>&1 
log pc_confinement.out
echo Done. 

# We've now got a stratafied program

# Let's output the modified binary
# This binary will really be a shell script that calls the newly stratafied binary

current_dir=`pwd`
peasoup_binary=$name.sh

echo "#!/bin/sh" >> $peasoup_binary
echo "" >> $peasoup_binary
echo "$PEASOUP_HOME/tools/ps_run.sh $current_dir \"\$@\"" >> $peasoup_binary 



chmod +x $peasoup_binary


echo Running IDA Pro static analysis phase ...
$SMPSA_HOME/SMP-analyze.sh a.ncexe
echo Done.

#
# Run concolic engine
#
echo Running concolic testing to generate inputs ...
#$PEASOUP_HOME/tools/do_concolic.sh a  --iterations 25 --logging tracer,instance_times,trace
$PEASOUP_HOME/tools/do_concolic.sh a  -t 600 -u 60 -i 25 -l tracer,trace,inputs  > do_concolic.out 2>&1
log do_concolic.out
echo Done.


#
# Populate IR Database
#
if [ ! "X" = "X"$PGUSER ]; then
	echo "Registering with IR database: program: $orig_exe server:$PGHOST db:$PGDATABASE"
	
	DB_PROGRAM_NAME=`basename $orig_exe.$$ | sed "s/[\.;+\\-\ ]/_/g"`
	
	MD5HASH=`md5sum a.ncexe | cut -f1 -d' '`
	$PEASOUP_HOME/tools/db/pdb_register.sh $DB_PROGRAM_NAME $current_dir  > pdb_register.out 2>&1 # register the program.
	varid=$?
	log pdb_register.out

	$PEASOUP_HOME/tools/db/pdb_create_program_tables.sh $DB_PROGRAM_NAME  > pdb_create_program_tables.out 2>&1 # create the tables for the program.
	log pdb_create_program_tables.out

    # check to see if annot file exists before doing anything
    if [ -f a.ncexe.annot ]; then

	    time $SECURITY_TRANSFORMS_HOME/tools/meds2pdb/meds2pdb $DB_PROGRAM_NAME a.ncexe $MD5HASH a.ncexe.annot 	 > meds2pdb.out 2>&1 # import meds information
	    log meds2pdb.out

	    if [ $varid -gt 0 ]; then
		    $SECURITY_TRANSFORMS_HOME/libIRDB/test/fill_in_cfg.exe $varid	> fill_in_cfg.out 	2>&1	# finish the initial IR by setting target/fallthrough 
		    log fill_in_cfg.out
		    $SECURITY_TRANSFORMS_HOME/libIRDB/test/fill_in_indtargs.exe $varid ./a.ncexe    > fill_in_indtargs.out 	2>&1 	# analyze for indirect branch targets 
		    log fill_in_indtargs.out
		    $SECURITY_TRANSFORMS_HOME/libIRDB/test/clone.exe $varid				> clone.out 		2>&1 	# create a clone
		    cloneid=$?
		    log clone.out
	echo "clone id is: $cloneid"
		    if [ $cloneid -gt 0 ]; then
															# paths for direct control transfers insns.
			$SECURITY_TRANSFORMS_HOME/libIRDB/test/fix_calls.exe $cloneid	> fix_calls.out 2>&1 		# fix call insns so they are OK for spri emitting
			log fix_calls.out
			
			mkdir p1.xform
			$PEASOUP_HOME/tools/cover.sh > cover.out 2>&1 #determine suitable coverage for functions to be p1-transformed
			
			# look for the coverage file, if absent, something didn't work (for now probably GraCE)
			if [ -f p1.xform/p1.coverage ]; then
				date > p1transform.out
				$SECURITY_TRANSFORMS_HOME/tools/transforms/p1transform.exe $cloneid p1.xform/p1.filtered_out >> p1transform.out 2>&1 
				date >> p1transform.out
				log p1transform.out
			else
				echo "No coverage file -- do not attempt P1 transform" > p1transform.out
			fi

			# reuse black list from p1 step (if present) o/w blacklist libc
			if [ -f p1.xform/p1.coverage ]; then
				BLACK_LIST=p1.xform/p1.filtered_out
			else
				BLACK_LIST="$PEASOUP_HOME/tools/p1xform.filter.libc.txt"
			fi
			$SECURITY_TRANSFORMS_HOME/tools/transforms/integerbugtransform.exe $cloneid "$BLACK_LIST" > integerbugtransform.out 2>&1
			log integerbugtransform.out

			$SECURITY_TRANSFORMS_HOME/libIRDB/test/ilr.exe $cloneid > ilr.out 2>&1 				# perform ILR 
#			log ilr.out

			$SECURITY_TRANSFORMS_HOME/libIRDB/test/generate_spri.exe $cloneid a.irdb.aspri	> spri.out 2>&1 # generate the aspri code
			log spri.out
			$SECURITY_TRANSFORMS_HOME/tools/spasm/spasm a.irdb.aspri a.irdb.bspri stratafier.o.exe > spasm.out 2>&1 	# generate the bspri code
			log spasm.out
		fi
	fi
	echo	-------------------------------------------------------------------------------
	echo    ---------            Orig Variant ID is $varid         ------------------------
	echo	-------------------------------------------------------------------------------
	echo    ---------            Cloned Variant ID is $cloneid     ------------------------
	echo	-------------------------------------------------------------------------------

    else
        # annotations file didn't exist
        echo "ERROR: annot file does not exist.  Not performing IRDB step"
        echo "Unable to create protected executable"
        exit -1
    fi

fi

# go back to original directory
cd - > /dev/null 2>&1

cp $newdir/$name.sh $stratafied_exe

# return the exit code of the copy as the final return value 
# So that a predictable return value is returned
retval=$?
exit $retval
