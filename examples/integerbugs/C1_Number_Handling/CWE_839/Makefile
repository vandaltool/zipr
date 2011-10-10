# DO NOT EDIT - This Makefile automatically generated
include ../../Makefile.options
include $(wildcard Makefile.options)

all: runall

test:
	../../../scripts/test.pl $(TESTFLAGS) .

CWE_839_Example_1_bad.ncexe: CWE_839_Example_1_bad.c
	$(CC) $(CFLAGS)  $< -o $@ $(LINKOPT)
	@echo $(CC) $(CFLAGS)  $< -o $@ $(LINKOPT) >CWE_839_Example_1_bad.conf

CWE_839_Example_1_bad: CWE_839_Example_1_bad.ncexe
	sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ 2>&1  | tee CWE_839_Example_1_bad.ps_analyze.log
	cp CWE_839_Example_1_bad.conf CWE_839_Example_1_bad.ncexe.conf
	@echo sh $(PEASOUP_HOME)/tools/ps_analyze.sh $< $@ >> CWE_839_Example_1_bad.conf

CWE_839_Example_1_bad.run: CWE_839_Example_1_bad
	../../../scripts/test.pl $(TESTFLAGS) CWE_839_Example_1_bad.ncexe

clean:
	rm -f  CWE_839_Example_1_bad.ncexe
	rm -f  CWE_839_Example_1_bad
	rm -f  summary.csv logfail.txt logfile.txt CWE_839_Example_1_bad.asm CWE_839_Example_1_bad.idb CWE_839_Example_1_bad.ncexe* stratafier.o.exe *.conf $(CLEANEXTRAS) *.ps_analyze.log
	rm -rf peasoup_executable_directory*

targets:  CWE_839_Example_1_bad.ncexe

analyze: targets CWE_839_Example_1_bad

runall:  CWE_839_Example_1_bad.run

