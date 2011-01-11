TOOLBASE=${PEASOUP_HOME}/tools

all: dumbledore.original dumbledore_cmd.original

dumbledore.original: dumbledore.o
	${TOOLBASE}/ps_link.sh dumbledore.o -o dumbledore.original

dumbledore_cmd.original: dumbledore_cmd.o
	${TOOLBASE}/ps_link.sh dumbledore_cmd.o -o dumbledore_cmd.original

.c.o:
	${TOOLBASE}/ps_comp.sh  $<

clean:
	rm *.o dumbledore.original dumbledore_cmd.original dumbledore.protected dumbledore_cmd.protected tmp
