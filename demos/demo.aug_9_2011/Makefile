TOOLBASE=${PEASOUP_HOME}/tools

all: dumbledore.original dumbledore.protected heap_overflow.original heap_overflow.protected

heap_overflow.original: heap_overflow.o
	gcc -g heap_overflow.o -o heap_overflow.original

heap_overflow.protected: heap_overflow.original
    # assume IDA Pro 6.0
	TVHEADLESS=1 ${TOOLBASE}/ps_analyze.sh heap_overflow.original heap_overflow.protected

dumbledore.original: dumbledore.o
	gcc -g dumbledore.o -o dumbledore.original

dumbledore.protected: dumbledore.original
    # assume IDA Pro 6.0
	TVHEADLESS=1 ${TOOLBASE}/ps_analyze.sh dumbledore.original dumbledore.protected

.c.o:
	${TOOLBASE}/ps_comp.sh  $<

clean:
	rm -f *.o heap_overflow.original heap_overflow.protected dumbledore.original dumbledore_cmd.original dumbledore.protected dumbledore_cmd.protected tmp
	rm -Rf peasoup_executable_directory.*
	# clean up IRDB and reset tables
	if [ ! "X" = "X"${PGUSER} ]; then sh ${TOOLBASE}/db/drop_my_tables.sh; sh ${TOOLBASE}/db/pdb_setup.sh; fi
