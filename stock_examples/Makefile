
CC=gcc
CXX=g++
CFLAGS=-O2

.SUFFIXES: .stock .protected

protected_exes=awk.protected bzip2.protected cal.protected cat.protected clear.protected date.protected df.protected diff.protected du.protected expr.protected find.protected grep.protected hostname.protected ipcs.protected less.protected ls.protected nasm.protected objdump.protected ps.protected sdiff.protected wc.protected whoami.protected xeyes.protected xcalc.protected xpdf.protected firefox-bin.protected lighttpd.protected mplayer.protected gnash.protected xpdfsrc.protected gcc-4.4.protected

ilr=on
stratafy_with_pc_confine=on
create_binary_script=on
heaprand=on
double_free=on
pc_confine=on
isr=off
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


all: env_check  ${protected_exes}

.PHONY: env_check 



.stock.protected: .PHONY
	${PEASOUP_HOME}/tools/ps_analyze.sh $< $@ \
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
                --step concolic=${concolic}\
                --step fix_calls=${fix_calls}\
                --step p1transform=${p1transform}\
                --step integertransform=${integertransform}\
                --step generate_spri=${generate_spri}\
                --step spasm=${spasm}\



$(protected_exes): ${STRATA}/lib/x86_linux/libstrata.a .PHONY



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
	@echo ----------------------------------------------------------
	@echo -------- Note that ISR is off by default here ------------
	@echo ----------------------------------------------------------


clean:
	rm -f *.o *.syms *.map chopzero hanoi hanoi_overrun hanoi_heap_overrun malloc block_copy print_ptr hanoi_stack_overrun dumbledore a.out memcpy hanoi_overrun_taintedenv dumbledore_cmd hanoi_overrun_tainted hello cmd_args_005
	rm -f *.exe *.dis *.data *.idb *.log *.ncexe *.annot *.readelf temp.* *.temp *.stratafied *.asm *.SMPobjdump *.id0 *.id1 *.til *.nam
	rm -Rf concolic.files_*
	rm -Rf peasoup_executable_directory.*
	rm -f strata.log.*
	rm -f *.sym
	rm -f *.protected

concclean:
	rm -Rf concolic.files_*
	rm strata.log.*

