# DO NOT EDIT - This Makefile automatically generated
include ../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../scripts/test.pl $(TESTFLAGS) .

CWE_682_Example_1_bad.ncexe: CWE_682_Example_1_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_682_Example_1_bad.conf

CWE_682_Example_1_bad: CWE_682_Example_1_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_682_Example_1_bad.ps_analyze.log
	cp CWE_682_Example_1_bad.conf CWE_682_Example_1_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_682_Example_1_bad.conf

CWE_682_Example_1_bad.run: CWE_682_Example_1_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_682_Example_1_bad.ncexe

CWE_682_Example_3_bad.ncexe: CWE_682_Example_3_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_682_Example_3_bad.conf

CWE_682_Example_3_bad: CWE_682_Example_3_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_682_Example_3_bad.ps_analyze.log
	cp CWE_682_Example_3_bad.conf CWE_682_Example_3_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_682_Example_3_bad.conf

CWE_682_Example_3_bad.run: CWE_682_Example_3_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_682_Example_3_bad.ncexe

clean:
	rm -f  CWE_682_Example_1_bad.ncexe CWE_682_Example_3_bad.ncexe
	rm -f  CWE_682_Example_1_bad CWE_682_Example_3_bad
	rm -f  summary.csv logfail.txt logfile.txt CWE_682_Example_1_bad.asm CWE_682_Example_1_bad.idb CWE_682_Example_1_bad.ncexe* stratafier.o.exe *.conf CWE_682_Example_3_bad.asm CWE_682_Example_3_bad.idb CWE_682_Example_3_bad.ncexe* stratafier.o.exe *.conf $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  CWE_682_Example_1_bad.ncexe CWE_682_Example_3_bad.ncexe

analyze: targets CWE_682_Example_1_bad CWE_682_Example_3_bad

runall:  CWE_682_Example_1_bad.run CWE_682_Example_3_bad.run

