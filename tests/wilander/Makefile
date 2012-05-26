CC=${TOOLCHAIN}/bin/gcc
INCLUDES=-I${STRATA}/include
PROTO2_OBJS=rev_memcpy.o dynamic_testbed_john_wilander.original.o
PROTO2_SRCS=rev_memcpy.c dynamic_testbed.john_wilander.original.c
PROTO2_LIBS=${STRATA}/lib/x86_linux/libstrata.a
progs=wilander_pn

#turning off fix_calls can cause problems in other programs so only
#turn it off for those vulnerable wilander functions. I believe it is safe
#in these cases, and allows for PN to catch more  attacks. 

ps_analyze_flags=" --step ilr=off\
               --step concolic=off\
	       --step fix_calls=off\
	       --step watchdog=off\
	       --step signconv_func_monitor=off\
               --step pc_confine=off\
               --step isr=off\
               --step meds_static=on\
               --step pdb_register=on\
               --step pdb_create_tables=on\
               --step meds2pdb=on\
               --step fill_in_cfg=on\
               --step fill_in_indtargs=on\
               --step clone=on\
               --step p1transform=on\
	       --step manual_test=on\
               --step integertransform=off\
               --step generate_spri=on\
               --step spasm=on "

wilander_tests.with_strata: .PHONY wilander_tests.with_strata.ncexe wilander_tests.with_strata.stratafied
	@echo wilander_tests.with_strata.built

wilander_tests.with_strata.stratafied: $(PROTO2_LIBS) wilander_tests.with_strata.ncexe
	sh ${STRATA_HOME}/tools/pc_confinement/stratafy_with_pc_confine.sh wilander_tests.with_strata.ncexe
	 mv new.exe wilander_tests.with_strata.stratafied
	${PEASOUP_HOME}/tools/generate_exe.sh wilander_tests.with_strata wilander_tests.with_strata.stratafied wilander_tests.with_strata.ncexe wilander_tests.with_strata.ncexe.annot

wilander_tests.with_strata.ncexe: $(PROTO2_OBJS)
	$(CC) -Bstatic -static $(PROTO2_OBJS) -o wilander_tests.with_strata.ncexe
	${SMPSA_HOME}/SMP-analyze.sh wilander_tests.with_strata.ncexe
	@if [ ! -f wilander_tests.with_strata.ncexe.annot ]; then echo Failed to generate annotations file; exit 1; fi 

wilander_tests.with_strata.o: $(PROTO2_SRCS)
	$(CC) -g $(PROTO2_SRCS) -c $(INCLUDES) 

wilander_tests.no_strata: $(PROTO2_OBJS)
	$(CC)  -Bstatic -static $(PROTO2_OBJS) -o wilander_tests.no_strata

#wilander_pn: $(PROTO2_OBJS)
wilander_pn: wilander_tests.no_strata
#	$(CC) -Bstatic -static $(PROTO2_OBJS) -o wilander_tests.with_strata.ncexe
	${PEASOUP_HOME}/tools/ps_analyze.sh wilander_tests.no_strata wilander_pn  ${ps_analyze_flags} --manual_test_script $(SECURITY_TRANSFORMS_HOME)/tests/simple/true_wrapper.sh

$(PROTO2_SRCS) $(DEFAULT_SRCS): rev_memcpy.h

.PHONY: env_check chopzero

# The generic rule for getting a .o file
.c.o:
	$(CC) -g -O0 $< -c $(INCLUDES)

chopzero:
	@ if [ ! -f ${PEASOUP_HOME}/chopzero_src/chopzero ]; then gcc ${PEASOUP_HOME}/chopzero_src/chopzero.c -o ${PEASOUP_HOME}/chopzero -O3 ; fi

env_check:
	@echo checking env vars; \
	if [ "X${TOOLCHAIN}" = "X" ]; then \
		echo TOOLCHAIN environment variable should be set.; \
		exit -1;\
	elif [ "X${STRATA}" = "X" ]; then \
		echo STRATA environment variable should be set. ;\
		exit -1;\
	elif [ "X${PEASOUP_HOME}" = "X" ]; then \
		echo PEASOUP_HOME environment variable should be set.; \
		exit -1;\
	elif [ "X${STRATA_HOME}" = "X" ]; then \
		echo STRATA_HOME environment variable should be set.; \
		exit -1;\
	fi ;


clean:
	rm -f ${progs}
	rm -Rf peasoup_executable_directory.*
	${PEASOUP_HOME}/tools/db/drop_my_tables.sh
	${PEASOUP_HOME}/tools/db/pdb_setup.sh
	rm -f wilander_tests.with_strata* wilander_tests.no_strata* *.o *.list *.map *.dis *-*isr *.asm *.exe tmp.exe.*
