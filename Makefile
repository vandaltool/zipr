



CC=${TOOLCHAIN}/bin/gcc -I ${STRATA}/include/
CXX=${TOOLCHAIN}/bin/g++ -I ${STRATA}/include/

all: env_check chopzero hanoi hanoi_overrun hanoi_heap_overrun hanoi_stack_overrun print_ptr malloc block_copy hello hanoi_overrun_tainted hanoi_overrun_taintedenv memcpy



#
# So, why is hanoi build this way, and the rest are built with nicecap link?  
# Debug time.  If it's built this way, the makefile figures out when the .annot file needs
# needs updating and when we just need to re-stratafy hanoi.  Since building the annot file takes
# 1-min+, and stratafying takes 15 seconds, this optimization is a big bonus in a development cycle.
#
# If someone knows a neat trick of makefiles to get all of the exe's to build this way, go for it.
# I expect some of the other tests will get cut/pasted versions of hanoi in the future.
#
#
hanoi:  .PHONY hanoi.ncexe hanoi.stratafied hanoi.ncexe.annot
	@echo hanoi built

hanoi.stratafied: ${STRATA}/lib/x86_linux/libstrata_normal.a hanoi.ncexe 
	${STRATAFIER}/do_stratafy.sh -k hanoi.ncexe 
	mv new.exe hanoi.stratafied
	${NICECAP_HOME}/generate_exe.sh hanoi hanoi.stratafied hanoi.ncexe hanoi.ncexe.annot 

hanoi.ncexe: hanoi.o 
	${TOOLCHAIN}/bin/gcc -Bstatic -static -O3 -fomit-frame-pointer hanoi.o -o hanoi.ncexe

hanoi.ncexe.annot: hanoi.ncexe ${IDAROOT}/plugins/SMPStaticAnalyzer.plx ${SMPSA_HOME}/SMP-analyze.sh
	${SMPSA_HOME}/SMP-analyze.sh hanoi.ncexe
	@if [ ! -f hanoi.ncexe.annot ]; then echo Failed to generate annotations file; exit 1; fi

globalfield:  .PHONY globalfield.ncexe globalfield.stratafied globalfield.ncexe.annot
	@echo globalfield built

globalfield.stratafied: ${STRATA}/lib/x86_linux/libstrata_normal.a globalfield.ncexe 
	${STRATA_HOME}/tools/stratafier/do_stratafy.sh globalfield.ncexe 
	mv new.exe globalfield.stratafied
	${NICECAP_HOME}/generate_exe.sh globalfield globalfield.stratafied globalfield.ncexe globalfield.ncexe.annot 

globalfield.ncexe: globalfield.o 
	${TOOLCHAIN}/bin/gcc -Bstatic -static -O3 -fomit-frame-pointer globalfield.o -o globalfield.ncexe

globalfield.ncexe.annot: globalfield.ncexe ${IDAROOT}/plugins/SMPStaticAnalyzer.plx ${SMPSA_HOME}/SMP-analyze.sh
	${SMPSA_HOME}/SMP-analyze.sh globalfield.ncexe
	@if [ ! -f globalfield.ncexe.annot ]; then echo Failed to generate annotations file; exit 1; fi


recover_example:  .PHONY recover_example.ncexe recover_example.stratafied recover_example.ncexe.annot
	@echo recover_example built

recover_example.stratafied: ${STRATA}/lib/x86_linux/libstrata_normal.a recover_example.ncexe
	${STRATA_HOME}/tools/stratafier/do_stratafy.sh recover_example.ncexe
	mv new.exe recover_example.stratafied
	${NICECAP_HOME}/generate_exe.sh recover_example recover_example.stratafied recover_example.ncexe recover_example.ncexe.annot

recover_example.ncexe: recover_example.o
	${TOOLCHAIN}/bin/gcc -Bstatic -static -O -fomit-frame-pointer recover_example.o -o recover_example.ncexe

recover_example.ncexe.annot: recover_example.ncexe ${IDAROOT}/plugins/SMPStaticAnalyzer.plx ${SMPSA_HOME}/SMP-analyze.sh
	${SMPSA_HOME}/SMP-analyze.sh recover_example.ncexe
	@if [ ! -f recover_example.ncexe.annot ]; then echo Failed to generate annotations file; exit 1; fi


memcpy:  .PHONY memcpy.ncexe memcpy.stratafied memcpy.ncexe.annot
	@echo memcpy built

memcpy.stratafied: ${STRATA}/lib/x86_linux/libstrata_normal.a memcpy.ncexe
	${STRATA_HOME}/tools/stratafier/do_stratafy.sh memcpy.ncexe
	mv new.exe memcpy.stratafied
	${NICECAP_HOME}/generate_exe.sh memcpy memcpy.stratafied memcpy.ncexe memcpy.ncexe.annot

memcpy.ncexe: memcpy.o
	${TOOLCHAIN}/bin/gcc -Bstatic -static -O3 -fomit-frame-pointer memcpy.o -o memcpy.ncexe

memcpy.ncexe.annot: memcpy.ncexe ${IDAROOT}/plugins/SMPStaticAnalyzer.plx ${SMPSA_HOME}/SMP-analyze.sh
	${SMPSA_HOME}/SMP-analyze.sh memcpy.ncexe
	@if [ ! -f memcpy.ncexe.annot ]; then echo Failed to generate annotations file; exit 1; fi



hanoi_overrun_tainted:  .PHONY hanoi_overrun_tainted.ncexe hanoi_overrun_tainted.stratafied hanoi_overrun_tainted.ncexe.annot
	@echo hanoi_overrun_tainted built

hanoi_overrun_tainted.stratafied: ${STRATA}/lib/x86_linux/libstrata_normal.a hanoi_overrun_tainted.ncexe
	${STRATA_HOME}/tools/stratafier/do_stratafy.sh hanoi_overrun_tainted.ncexe
	mv new.exe hanoi_overrun_tainted.stratafied
	${NICECAP_HOME}/generate_exe.sh hanoi_overrun_tainted hanoi_overrun_tainted.stratafied hanoi_overrun_tainted.ncexe hanoi_overrun_tainted.ncexe.annot

hanoi_overrun_tainted.ncexe: hanoi_overrun_tainted.o
	${TOOLCHAIN}/bin/gcc -Bstatic -static hanoi_overrun_tainted.o -o hanoi_overrun_tainted.ncexe

hanoi_overrun_tainted.ncexe.annot: hanoi_overrun_tainted.ncexe ${IDAROOT}/plugins/SMPStaticAnalyzer.plx ${SMPSA_HOME}/SMP-analyze.sh
	${SMPSA_HOME}/SMP-analyze.sh hanoi_overrun_tainted.ncexe
	@if [ ! -f hanoi_overrun_tainted.ncexe.annot ]; then echo Failed to generate annotations file; exit 1; fi


hanoi_overrun_taintedenv:  .PHONY hanoi_overrun_taintedenv.ncexe hanoi_overrun_taintedenv.stratafied hanoi_overrun_taintedenv.ncexe.annot
	@echo hanoi_overrun_taintedenv built

hanoi_overrun_taintedenv.stratafied: ${STRATA}/lib/x86_linux/libstrata_normal.a hanoi_overrun_taintedenv.ncexe
	${STRATA_HOME}/tools/stratafier/do_stratafy.sh hanoi_overrun_taintedenv.ncexe
	mv new.exe hanoi_overrun_taintedenv.stratafied
	${NICECAP_HOME}/generate_exe.sh hanoi_overrun_taintedenv hanoi_overrun_taintedenv.stratafied hanoi_overrun_taintedenv.ncexe hanoi_overrun_taintedenv.ncexe.annot

hanoi_overrun_taintedenv.ncexe: hanoi_overrun_taintedenv.o
	${TOOLCHAIN}/bin/gcc -Bstatic -static hanoi_overrun_taintedenv.o -o hanoi_overrun_taintedenv.ncexe

hanoi_overrun_taintedenv.ncexe.annot: hanoi_overrun_taintedenv.ncexe ${IDAROOT}/plugins/SMPStaticAnalyzer.plx ${SMPSA_HOME}/SMP-analyze.sh
	${SMPSA_HOME}/SMP-analyze.sh hanoi_overrun_taintedenv.ncexe
	@if [ ! -f hanoi_overrun_taintedenv.ncexe.annot ]; then echo Failed to generate annotations file; exit 1; fi



hanoi++:  .PHONY hanoi++.ncexe hanoi++.stratafied
	@echo hanoi++ built

hanoi++.stratafied: ${STRATA}/lib/x86_linux/libstrata_normal.a hanoi++.ncexe
	${STRATA_HOME}/tools/stratafier/do_stratafy.sh hanoi++.ncexe
	mv new.exe hanoi++.stratafied
	${NICECAP_HOME}/generate_exe.sh hanoi++ hanoi++.stratafied hanoi++.ncexe hanoi++.ncexe.annot

hanoi++.ncexe: hanoi++.o
	${TOOLCHAIN}/bin/g++ -Bstatic -static hanoi++.o -o hanoi++.ncexe
	${SMPSA_HOME}/SMP-analyze.sh hanoi++.ncexe
	@if [ ! -f hanoi++.ncexe.annot ]; then echo Failed to generate annotations file; exit 1; fi

hanoi_overrun: hanoi_overrun.o ${STRATA}/lib/x86_linux/libstrata_normal.a .PHONY
	./nicecap_link hanoi_overrun.o -o hanoi_overrun
	${SMPSA_HOME}/SMP-analyze.sh hanoi_overrun.ncexe

hanoi_heap_overrun: hanoi_heap_overrun.o ${STRATA}/lib/x86_linux/libstrata_normal.a .PHONY
	./nicecap_link hanoi_heap_overrun.o -o hanoi_heap_overrun
	${SMPSA_HOME}/SMP-analyze.sh hanoi_heap_overrun.ncexe


hanoi_stack_overrun: hanoi_stack_overrun.o ${STRATA}/lib/x86_linux/libstrata_normal.a .PHONY
	./nicecap_link hanoi_stack_overrun.o -o hanoi_stack_overrun
	${SMPSA_HOME}/SMP-analyze.sh hanoi_stack_overrun.ncexe

print_ptr: print_ptr.o ${STRATA}/lib/x86_linux/libstrata_normal.a .PHONY
	./nicecap_link print_ptr.o -o print_ptr
	${SMPSA_HOME}/SMP-analyze.sh print_ptr.ncexe

block_copy: block_copy.o ${STRATA}/lib/x86_linux/libstrata_normal.a .PHONY
	./nicecap_link block_copy.o -o block_copy
	${SMPSA_HOME}/SMP-analyze.sh block_copy.ncexe

malloc: malloc.o ${STRATA}/lib/x86_linux/libstrata_normal.a .PHONY
	./nicecap_link malloc.o -o malloc
	${SMPSA_HOME}/SMP-analyze.sh malloc.ncexe

hello: hello.o ${STRATA}/lib/x86_linux/libstrata_normal.a .PHONY
	./nicecap_link hello.o -o hello
	${SMPSA_HOME}/SMP-analyze.sh hello.ncexe

.PHONY: env_check chopzero

.c.o:
	$(CC) -O3 -fomit-frame-pointer $< -c

.cpp.o:
	$(CXX) -O3 $< -c

chopzero:
	@ if [ ! -f chopzero ]; then gcc chopzero.c -o chopzero -O3 ; fi



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
 	elif [ "X${NICECAP_HOME}" = "X" ]; then \
		echo NICECAP_HOME environment variable should be set.; \
		exit -1;\
 	elif [ "X${STRATA_HOME}" = "X" ]; then \
		echo STRATA_HOME environment variable should be set.; \
		exit -1;\
	fi ; 


double_free_suite:
	cd double_free_tests; make

clean:
	rm -f *.o *.syms *.map chopzero hanoi hanoi_overrun hanoi_heap_overrun malloc block_copy print_ptr hanoi_stack_overrun
	rm -f *.exe *.dis *.data *.idb *.log *.ncexe *.annot *.readelf temp.* *.temp *.stratafied *.asm *.SMPobjdump
