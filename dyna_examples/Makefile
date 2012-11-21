



CC=gcc
CXX=g++
CFLAGS=-O -w
CXXFLAGS=-O -w
LD=gcc
LDFLAGS=

.SUFFIXES: .o .c .cpp .stock .protected

exes=hanoi.protected hanoi_overrun.protected hanoi_heap_overrun.protected hanoi_stack_overrun.protected print_ptr.protected malloc.protected block_copy.protected hello.protected hanoi_overrun_tainted.protected hanoi_overrun_taintedenv.protected memcpy.protected cmd_args_005.protected dumbledore_stdin.protected dumbledore_cmd.protected dumbledore_file.protected 000.protected long.protected bad_purify.protected

ilr=on
stratafy_with_pc_confine=on
create_binary_script=on
heaprand=on
double_free=on
pc_confine=on
isr=on
meds_static=on
pdb_register=on
pdb_create_tables=on
meds2pdb=on
fill_in_cfg=on
fill_in_indtargs=on
clone=on
fix_calls=on
p1transform=on
integertransform=on
ilr=on
generate_spri=on
spasm=on
concolic=on
watchdog=on



all: env_check  ${exes}



.PHONY: env_check 


.o.stock:
	${LD} ${LDFLAGS} $< -o $@ 

.stock.protected:
	${PEASOUP_HOME}/tools/ps_analyze.sh  $< $@ \
		--step ilr=${ilr}\
		--step stratafy_with_pc_confine=${stratafy_with_pc_confine}\
		--step create_binary_script=${create_binary_script}\
		--step heaprand=${heaprand}\
		--step double_free=${double_free}\
		--step pc_confine=${pc_confine}\
		--step isr=${isr}\
		--step meds_static=${meds_static}\
		--step pdb_register=${pdb_register}\
		--step pdb_create_tables=${pdb_create_tables}\
		--step meds2pdb=${meds2pdb}\
		--step fill_in_cfg=${fill_in_cfg}\
		--step fill_in_indtargs=${fill_in_indtargs}\
		--step clone=${clone}\
		--step fix_calls=${fix_calls}\
		--step p1transform=${p1transform}\
		--step integertransform=${integertransform}\
		--step generate_spri=${generate_spri}\
		--step spasm=${spasm}\
		--step watchdog=${watchdog}\
		--step concolic=${concolic}\


.c.o:
	${PEASOUP_HOME}/tools/ps_comp.sh $< 
	${CC} ${CFLAGS} -c $< 

.cpp.o:
	${CXX} ${CXXFLAGS} -c $< 

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
	rm -f *.exe *.dis *.data *.idb *.log *.ncexe *.annot *.readelf temp.* *.temp *.stratafied *.asm *.SMPobjdump *.id0 *.id1 *.til *.nam *.stock *.protected
	rm -Rf concolic.files_*
	rm -Rf peasoup_executable_directory.*
	rm -f strata.log.*
	rm -f *.sym
	if [ ! "X" = "X"${PGUSER} ]; then sh ../tools/db/drop_my_tables.sh; sh ../tools/db/pdb_setup.sh; fi

concclean:
	rm -Rf concolic.files_*
	rm strata.log.*
