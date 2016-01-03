#!/bin/bash 
# analyze cal program

PROG=/usr/bin/cal
PROG_BASE=$(basename ${PROG})
TMPDIR=peasoup.${PROG_BASE}.tmp
TRACE_FILE=${PROG_BASE}.trace
XREFS=${PROG_BASE}.xrefs
PROG_SCFI=${PROG_BASE}.zipr.scfi.color

if [ -e "$TMPDIR" ]; then
	rm -fr $TMPDIR
fi

FIX_CALLS_FIX_ALL_CALLS=1 $PEASOUP_HOME/tools/ps_analyze.sh $PROG $PROG_SCFI --backend zipr --tempdir ${TMPDIR} --step selective_cfi=on --step-option selective_cfi:--color # --stop_after pdb_register

cp ${TMPDIR}/a.ncexe.STARSxrefs $XREFS

# invoke QEMU to get trace rm $TRACE_FILE
touch $TRACE_FILE

# arguments for cal
ARGS=("" "-h" "-bogus" "2000" "-w 2000" "-3 2001" "1 2000" "1 1999" "-j 2000" "-J 2000" "-S 1000" "-M 1015" "-J -o 12 9999" "-e -p -y 2000" "-M -y -B9 -A5 2000" "-S -y -B10 -A10 2000" "-1" "-A-20 -B-5 1000"  "-w -m 3 1111" "-N 2000" "-N -w 2000" "-N -3 2001" "-N 1 2000" "-N 1 1999" "-N -j 2000" "-N -J 2000" "-N -S 1000" "-N -M 1015" "-N -J -o 12 9999" "-N -e -p -y 2000" "-N -M -y -B9 -A5 2000" "-N -S -y -B10 -A10 2000" "-N -1" "-N -A-20 -B-5 1000"  "-N -w -m 3 1111" "-N -y 1999 -m 3" "-N -y -b")

for ix in ${!ARGS[*]}
do
	echo "args: ${ARGS[$ix]}" > ${TRACE_FILE}.${ix}
	qemu-x86_64 -singlestep -d in_asm $PROG ${ARGS[$ix]} >/dev/null 2>> tmp.$$
	grep "0x" tmp.$$ >> ${TRACE_FILE}.${ix}
	rm tmp.$$

	${PROG} ${ARGS[$ix]} >tmp.orig.out 2>tmp.orig.err
	./${PROG_SCFI} ${ARGS[$ix]} >tmp.cfi.out 2>tmp.cfi.err
	diff tmp.orig.out tmp.cfi.out >/dev/null 2>&1
	if [ ! $? -eq 0 ]; then
		echo "SCFI FAILURE"
		exit 1
	fi
	
done

# check IB Targets traced against spec in STARS xref file
TRACE_FAILED=""
for t in `ls ${TRACE_FILE}*`
do
	cat $t >> $TRACE_FILE
	args=$(head -1 $t)
	echo
	echo IBT Check for inputs: trace_file: $t invoked with: $args
	python ibtcheck.py $XREFS $t
	if [ ! $? -eq 0 ]; then
		echo FAILED: $args trace_file: $t
		TRACE_FAILED="$TRACE_FAILED [$t $args]"
	fi

done

echo
echo "Aggregate Statistics"
python ibtcheck.py $XREFS $TRACE_FILE

if [ ! -z "$TRACE_FAILED" ]; then
	echo
	echo "==============================================="
	echo "IBT check on trace failed: $TRACE_FAILED"  
	echo "==============================================="
	echo	
	exit 1
fi


exit 0

