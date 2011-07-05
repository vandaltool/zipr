



CC=DO_NOT_USE
CXX=DO_NOT_USE
CFLAGS=DO_NOT_USE
LD=DO_NOT_USE

.SUFFIXES: .o .c .cpp .exe

exes=hanoi.exe hanoi_overrun.exe hanoi_heap_overrun.exe hanoi_stack_overrun.exe print_ptr.exe malloc.exe block_copy.exe hello.exe hanoi_overrun_tainted.exe hanoi_overrun_taintedenv.exe memcpy.exe cmd_args_005.exe dumbledore_stdin.exe dumbledore_cmd.exe dumbledore_file.exe


all: env_check  ${exes}



.PHONY: env_check 


.o.exe:
	${PEASOUP_HOME}/tools/ps_link.sh $< -o $@ 
	${PEASOUP_HOME}/tools/ps_analyze.sh  $@ $@

.c.o:
	${PEASOUP_HOME}/tools/ps_comp.sh $< 

.cpp.o:
	${PEASOUP_HOME}/tools/ps_comp++.sh $< 

chopzero:
	@ if [ ! -f chopzero ]; then gcc chopzero.c -o chopzero -O3 ; fi

$(exes): ${STRATA}/lib/x86_linux/libstrata.a



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
	rm -f *.o *.syms *.map chopzero hanoi hanoi_overrun hanoi_heap_overrun malloc block_copy print_ptr hanoi_stack_overrun dumbledore a.out memcpy hanoi_overrun_taintedenv dumbledore_cmd hanoi_overrun_tainted hello cmd_args_005
	rm -f *.exe *.dis *.data *.idb *.log *.ncexe *.annot *.readelf temp.* *.temp *.stratafied *.asm *.SMPobjdump *.id0 *.id1 *.til *.nam
	rm -Rf concolic.files_*
	rm -Rf peasoup_executable_directory.*
	rm -f strata.log.*
	rm -f *.sym
	# if [ ! "X" = "X"${PGUSER} ]; then sh ../tools/db/drop_my_tables.sh; sh ../tools/db/pdb_setup.sh; fi

concclean:
	rm -Rf concolic.files_*
	rm strata.log.*
