# DO NOT EDIT - This Makefile automatically generated
include ../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../scripts/test.pl $(TESTFLAGS) .

CWE_190_Example_1_bad.ncexe: CWE_190_Example_1_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_190_Example_1_bad.conf

CWE_190_Example_1_bad: CWE_190_Example_1_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_190_Example_1_bad.ps_analyze.log
	cp CWE_190_Example_1_bad.conf CWE_190_Example_1_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_190_Example_1_bad.conf

CWE_190_Example_1_bad.run: CWE_190_Example_1_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_190_Example_1_bad.ncexe

CWE_190_Example_2_bad.ncexe: CWE_190_Example_2_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_190_Example_2_bad.conf

CWE_190_Example_2_bad: CWE_190_Example_2_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_190_Example_2_bad.ps_analyze.log
	cp CWE_190_Example_2_bad.conf CWE_190_Example_2_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_190_Example_2_bad.conf

CWE_190_Example_2_bad.run: CWE_190_Example_2_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_190_Example_2_bad.ncexe

modular_bug_finding_example_1.ncexe: modular_bug_finding_example_1.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >modular_bug_finding_example_1.conf

modular_bug_finding_example_1: modular_bug_finding_example_1.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee modular_bug_finding_example_1.ps_analyze.log
	cp modular_bug_finding_example_1.conf modular_bug_finding_example_1.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> modular_bug_finding_example_1.conf

modular_bug_finding_example_1.run: modular_bug_finding_example_1
	../../../scripts/test.pl $(TESTFLAGS) modular_bug_finding_example_1.ncexe

modular_bug_finding_example_2.ncexe: modular_bug_finding_example_2.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >modular_bug_finding_example_2.conf

modular_bug_finding_example_2: modular_bug_finding_example_2.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee modular_bug_finding_example_2.ps_analyze.log
	cp modular_bug_finding_example_2.conf modular_bug_finding_example_2.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> modular_bug_finding_example_2.conf

modular_bug_finding_example_2.run: modular_bug_finding_example_2
	../../../scripts/test.pl $(TESTFLAGS) modular_bug_finding_example_2.ncexe

UVA_C1_and_C4.ncexe: UVA_C1_and_C4.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >UVA_C1_and_C4.conf

UVA_C1_and_C4: UVA_C1_and_C4.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee UVA_C1_and_C4.ps_analyze.log
	cp UVA_C1_and_C4.conf UVA_C1_and_C4.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> UVA_C1_and_C4.conf

UVA_C1_and_C4.run: UVA_C1_and_C4
	../../../scripts/test.pl $(TESTFLAGS) UVA_C1_and_C4.ncexe

clean:
	rm -f  CWE_190_Example_1_bad.ncexe CWE_190_Example_2_bad.ncexe modular_bug_finding_example_1.ncexe modular_bug_finding_example_2.ncexe UVA_C1_and_C4.ncexe
	rm -f  CWE_190_Example_1_bad CWE_190_Example_2_bad modular_bug_finding_example_1 modular_bug_finding_example_2 UVA_C1_and_C4
	rm -f  summary.csv logfail.txt logfile.txt CWE_190_Example_1_bad.asm CWE_190_Example_1_bad.idb CWE_190_Example_1_bad.ncexe* stratafier.o.exe *.conf CWE_190_Example_2_bad.asm CWE_190_Example_2_bad.idb CWE_190_Example_2_bad.ncexe* stratafier.o.exe *.conf modular_bug_finding_example_1.asm modular_bug_finding_example_1.idb modular_bug_finding_example_1.ncexe* stratafier.o.exe *.conf modular_bug_finding_example_2.asm modular_bug_finding_example_2.idb modular_bug_finding_example_2.ncexe* stratafier.o.exe *.conf UVA_C1_and_C4.asm UVA_C1_and_C4.idb UVA_C1_and_C4.ncexe* stratafier.o.exe *.conf $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  CWE_190_Example_1_bad.ncexe CWE_190_Example_2_bad.ncexe modular_bug_finding_example_1.ncexe modular_bug_finding_example_2.ncexe UVA_C1_and_C4.ncexe

analyze: targets CWE_190_Example_1_bad CWE_190_Example_2_bad modular_bug_finding_example_1 modular_bug_finding_example_2 UVA_C1_and_C4

runall:  CWE_190_Example_1_bad.run CWE_190_Example_2_bad.run modular_bug_finding_example_1.run modular_bug_finding_example_2.run UVA_C1_and_C4.run

