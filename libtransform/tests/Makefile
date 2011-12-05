CC=DO_NOT_USE
CXX=DO_NOT_USE
CFLAGS=DO_NOT_USE
LD=DO_NOT_USE

.SUFFIXES: .o .c .cpp .exe

#exes=dumbledore_cmd.exe test1.exe
#exes=overflow1.exe #overflow2.exe
#exes=dumbledore_cmd.exe
exes=integerbug.exe


all: env_check  ${exes}

.PHONY: env_check 

.o.exe:
#	${PEASOUP_HOME}/tools/ps_link.sh $< -o $@ 
#	gcc -g $< -o $@ 
	gcc $< -o $@ 
	${PEASOUP_HOME}/tools/ps_analyze.sh  $@ $@.peasoup --step ilr=off --step concolic=off --step p1transform=off --step isr=off
#	${PEASOUP_HOME}/tools/ps_analyze.sh  $@ $@.peasoup --step ilr=off --step concolic=off --step p1transform=off --step isr=off

.c.o:
	gcc -O2 -c $< 
#	${PEASOUP_HOME}/tools/ps_comp.sh $< 

.cpp.o:
	${PEASOUP_HOME}/tools/ps_comp++.sh $< 


env_check:
	@echo checking env vars; \
	if [ "X${TOOLCHAIN}" = "X" ]; then \
		echo TOOLCHAIN environment variable should be set.; \
		exit -1;\
 	elif [ "X${STRATA}" = "X" ]; then \
		echo STRATA environment variable should be set. ;\
		exit -1;\
 	elif [ "X${SMPSA_HOME}" = "X" ]; then \
		echo SMPSA_HOME environment variable should be set.; \
		exit -1;\
 	elif [ "X${PEASOUP_HOME}" = "X" ]; then \
		echo PEASOUP_HOME environment variable should be set.; \
		exit -1;\
 	elif [ "X${STRATA_HOME}" = "X" ]; then \
		echo STRATA_HOME environment variable should be set.; \
		exit -1;\
	fi ; 


clean:
	rm -f *.o *.syms *.map 
	rm -f *.exe *.dis *.data *.idb *.log *.ncexe *.readelf temp.* *.temp *.stratafied *.asm *.SMPobjdump *.id0 *.id1 *.til *.nam
	rm -Rf concolic.files_*
	rm -Rf peasoup_executable_directory.*
	rm -f strata.log.*
	rm -f *.sym
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh

concclean:
	rm -Rf concolic.files_*
	rm strata.log.*
