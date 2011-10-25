# DO NOT EDIT - This Makefile automatically generated
include ../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../scripts/test.pl $(TESTFLAGS) .

CWE_194_Example_1_bad.ncexe: CWE_194_Example_1_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_194_Example_1_bad.conf

CWE_194_Example_1_bad: CWE_194_Example_1_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_194_Example_1_bad.ps_analyze.log
	cp CWE_194_Example_1_bad.conf CWE_194_Example_1_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_194_Example_1_bad.conf

CWE_194_Example_1_bad.run: CWE_194_Example_1_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_194_Example_1_bad.ncexe

clean:
	rm -f  CWE_194_Example_1_bad.ncexe
	rm -f  CWE_194_Example_1_bad
	rm -f  summary.csv logfail.txt logfile.txt CWE_194_Example_1_bad.asm CWE_194_Example_1_bad.idb CWE_194_Example_1_bad.ncexe* stratafier.o.exe *.conf $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  CWE_194_Example_1_bad.ncexe

analyze: targets CWE_194_Example_1_bad

runall:  CWE_194_Example_1_bad.run

