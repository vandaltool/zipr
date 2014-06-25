CC=DO_NOT_USE
CXX=DO_NOT_USE
CFLAGS=DO_NOT_USE
LD=DO_NOT_USE

.SUFFIXES: .o .c .cpp .exe

#exes=dumbledore_cmd.exe test1.exe
#exes=overflow1.exe
#exes=dumbledore_cmd.exe
#exes=unsigned_mul.64.exe 
#exes=signed_add.exe
#exes=simpletest.exe
#exes=dummy.exe
#exes=unsigned_mul.64.exe signed_add.exe signed_mul.64.exe signed_mul.32.exe

exes=signed_add.64.exe signed_mul.64.exe unsigned_mul.64.exe signed_mul.32.exe
exes=trunc.64.32.signed.exe trunc.64.32.unsigned.exe trunc.64.16.signed.exe trunc.64.16.unsigned.exe trunc.64.8.signed.exe trunc.64.8.unsigned.exe
exes=trunc.32.16.signed.exe trunc.32.16.unsigned.exe
exes=trunc.32.16.signed.exe 

all: env_check  ${exes}

.PHONY: env_check 

.o.exe: 
#	${PEASOUP_HOME}/tools/ps_link.sh $< -o $@ 
	gcc -g $< -o $@ 
#	gcc -m32 $< -o $@ 
	${PEASOUP_HOME}/tools/ps_analyze64.sh  $@ $@.peasoup --step ilr=off --step concolic=off --step p1transform=off --step isr=off --integer_instrument_idioms

.c.o:
	gcc -O2 -c $< 
#	gcc -c -m32 $< 
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
	rm -f *.exe *.exe.peasoup *.dis *.data *.idb *.log *.ncexe *.readelf temp.* *.temp *.stratafied *.asm *.SMPobjdump *.id0 *.id1 *.til *.nam
	rm -Rf concolic.files_*
	rm -Rf peasoup_executable_directory.*
	rm -f strata.log.*
	rm -f *.sym
	rm -f *.peasoup
	rm -f *.tmp.* tmp.*

cleandb:
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh

concclean:
	rm -Rf concolic.files_*
	rm strata.log.*
