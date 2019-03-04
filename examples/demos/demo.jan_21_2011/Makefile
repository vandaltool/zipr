TOOLBASE=${PEASOUP_HOME}/tools

all: dumbledore.original dumbledore_cmd.original dumbledore.protected

dumbledore.original: dumbledore.o
	${TOOLBASE}/ps_link.sh dumbledore.o -o dumbledore.original

dumbledore_cmd.original: dumbledore_cmd.o
	${TOOLBASE}/ps_link.sh dumbledore_cmd.o -o dumbledore_cmd.original

dumbledore.protected: dumbledore.original
	${TOOLBASE}/ps_analyze.sh dumbledore.original dumbledore.protected
	echo "Note: this step will end in an error for now, but that's OK"

.c.o:
	${TOOLBASE}/ps_comp.sh  $<

clean:
	rm -f *.o dumbledore.original dumbledore_cmd.original dumbledore.protected dumbledore_cmd.protected tmp

	rm -Rf peasoup_executable_directory.*
