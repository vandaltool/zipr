



CC=gcc
CXX=g++
CFLAGS=-O2

.SUFFIXES: .o .c .cpp .exe .stock .protected

exes=hanoi.exe hanoi_overrun.exe hanoi_heap_overrun.exe hanoi_stack_overrun.exe print_ptr.exe malloc.exe block_copy.exe hello.exe hanoi_overrun_tainted.exe hanoi_overrun_taintedenv.exe memcpy.exe cmd_args_005.exe dumbledore_stdin.exe dumbledore_cmd.exe dumbledore_file.exe

protected_exes=awk.protected bzip2.protected cal.protected cat.protected clear.protected date.protected df.protected diff.protected du.protected expr.protected find.protected grep.protected hostname.protected ipcs.protected less.protected ls.protected nasm.protected objdump.protected ps.protected sdiff.protected wc.protected whoami.protected xeyes.protected

all: env_check  ${exes}  ${protected_exes}



.PHONY: env_check 

.stock.protected: .PHONY
	${PEASOUP_HOME}/tools/ps_analyze.sh $< $@

.o.exe: .PHONY
	$(CC) $< -o $@ 
	${PEASOUP_HOME}/tools/ps_analyze.sh  $@ $@

.c.o: .PHONY
	${CC} $<  -c

.cpp.o: .PHONY
	${CXX} $< 

chopzero: .PHONY
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
	rm -f *.protected
	if [ ! "X" = "X"${PGUSER} ]; then sh ../tools/db/drop_my_tables.sh; sh ../tools/db/pdb_setup.sh; fi

concclean:
	rm -Rf concolic.files_*
	rm strata.log.*

