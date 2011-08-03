TOOLBASE=${PEASOUP_HOME}/tools

all: dumbledore.original dumbledore.protected heap_overflow.original heap_overflow.protected

heap_overflow.original: heap_overflow.o
	${TOOLBASE}/ps_link.sh heap_overflow.o -o heap_overflow.original

heap_overflow.protected: heap_overflow.original
	${TOOLBASE}/ps_analyze.sh heap_overflow.original heap_overflow.protected
	echo "Note: this step will end in an error for now, but that's OK"

dumbledore.original: dumbledore.o
	${TOOLBASE}/ps_link.sh dumbledore.o -o dumbledore.original

dumbledore.protected: dumbledore.original
	${TOOLBASE}/ps_analyze.sh dumbledore.original dumbledore.protected
	echo "Note: this step will end in an error for now, but that's OK"

.c.o:
	${TOOLBASE}/ps_comp.sh  $<

clean:
	rm -f *.o heap_overflow.original heap_overflow.protected dumbledore.original dumbledore_cmd.original dumbledore.protected dumbledore_cmd.protected tmp
	rm -Rf peasoup_executable_directory.*
	# clean up IRDB and reset tables
	if [ ! "X" = "X"${PGUSER} ]; then sh ${TOOLBASE}/db/drop_my_tables.sh; sh ${TOOLBASE}/db/pdb_setup.sh; fi
